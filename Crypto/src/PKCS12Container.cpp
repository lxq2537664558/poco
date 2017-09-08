//
// PKCS12Container.cpp
//
// $Id: //poco/1.4/Crypto/src/PKCS12Container.cpp#1 $
//
// Library: Crypto
// Package: Certificate
// Module:  PKCS12Container
//
// Copyright (c) 2006-2009, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// SPDX-License-Identifier:	BSL-1.0
//


#include "Poco/Crypto/PKCS12Container.h"
#include "Poco/NumberFormatter.h"
#include "Poco/StreamCopier.h"
#include <sstream>
#include <openssl/err.h>


namespace Poco {
namespace Crypto {


PKCS12Container::PKCS12Container(std::istream& istr, const std::string& password)
{
	std::ostringstream ostr;
	Poco::StreamCopier::copyStream(istr, ostr);
	std::string cont = ostr.str();

	BIO *pBIO = BIO_new_mem_buf(const_cast<char*>(cont.data()), static_cast<int>(cont.size()));
	if (pBIO)
	{
		PKCS12* pkcs12 = d2i_PKCS12_bio(pBIO, NULL);
		BIO_free(pBIO);
		load(pkcs12, password);
	}
	else
	{
		throw NullPointerException("PKCS12Container BIO memory buffer");
	}
}


PKCS12Container::PKCS12Container(const Poco::Path& path, const std::string& password)
{
	FILE* pFile = nullptr;

	if ((pFile = fopen(path.toString().c_str(), "rb")))
	{
		PKCS12* pPKCS12 = d2i_PKCS12_fp(pFile, NULL);
		fclose (pFile);
		load(pPKCS12, password);
	}
	else
	{
		throw NullPointerException("PKCS12Container file:" + path.toString());
	}
}


PKCS12Container::PKCS12Container(const std::string& str, const std::string& password)
{
}


PKCS12Container::PKCS12Container(const PKCS12Container& cont)
{
}


PKCS12Container& PKCS12Container::operator = (const PKCS12Container& cert)
{
	return *this;
}


PKCS12Container::~PKCS12Container()
{
	if (_pKey) EVP_PKEY_free(_pKey);
}


void PKCS12Container::load(PKCS12* pPKCS12, const std::string& password)
{
	if (pPKCS12)
	{
		X509* pCert = nullptr;
		STACK_OF(X509)* pCA = nullptr;
		STACK_OF(PKCS12_SAFEBAG)* bags = nullptr;
		if (PKCS12_parse(pPKCS12, password.c_str(), &_pKey, &pCert, &pCA))
		{
			if (pCert && _pKey)
			{
				_pX509Cert.reset(new X509Certificate(pCert));
				PKCS12_SAFEBAG* bag = PKCS12_add_cert(&bags, pCert);
				char* buffer = PKCS12_get_friendlyname(bag);
				if (buffer) _pkcsFriendlyname = buffer;
				else _pkcsFriendlyname.clear();
				_caCertList.clear();
				if (pCA)
				{
					int certCount = sk_X509_num(pCA);
					for (int i = 0; i < certCount; ++i)
					{
						_caCertList.push_back(X509Certificate(sk_X509_value(pCA, i)));
					}
				}
			}
			else
			{
				_pX509Cert.reset();
			}
		}
		else
		{
			unsigned long e = ERR_get_error();
			char buf[128] = { 0 };
			char* pErr = ERR_error_string(e, buf);
			std::string err = "PKCS12Container PKCS12_parse error: ";
			if (pErr) err.append(pErr);
			else err.append(NumberFormatter::format(e));

			throw RuntimeException(err);
		}
		PKCS12_free(pPKCS12);
		sk_X509_pop_free(pCA, X509_free);
		X509_free(pCert);
	}
	else
	{
		throw NullPointerException("PKCS12Container: struct PKCS12");
	}
}


} } // namespace Poco::Crypto