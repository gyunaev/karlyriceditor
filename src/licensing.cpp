/**************************************************************************
 *  Karlyriceditor - a lyrics editor and CD+G / video export for Karaoke  *
 *  songs.                                                                *
 *  Copyright (C) 2009-2011 George Yunaev, support@karlyriceditor.com     *
 *                                                                        *
 *  This program is free software: you can redistribute it and/or modify  *
 *  it under the terms of the GNU General Public License as published by  *
 *  the Free Software Foundation, either version 3 of the License, or     *
 *  (at your option) any later version.                                   *
 *																	      *
 *  This program is distributed in the hope that it will be useful,       *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *  GNU General Public License for more details.                          *
 *                                                                        *
 *  You should have received a copy of the GNU General Public License     *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 **************************************************************************/

#include <QDate>
#include <QString>
#include <QSettings>

#if defined (USE_LICENSING)
	#include <openssl/x509.h>
	#include <openssl/x509_vfy.h>
#endif

#include "licensing.h"

Licensing * pLicensing;


static const char * CA_DER_CERT = "MIICkDCCAfmgAwIBAgIJAJvgo443LFCmMA0GCSqGSIb3DQEBBQUAMIGAMTEwLwYD"
									"VQQDDChrYXJseXJpY2VkaXRvci5jb20uY2VydGlmaWNhdGUuYXV0aG9yaXR5MRMw"
									"EQYDVQQIDApDYWxpZm9ybmlhMQswCQYDVQQGEwJVUzEpMCcGCSqGSIb3DQEJARYa"
									"c3VwcG9ydEBrYXJseXJpY2VkaXRvci5jb20wHhcNMTEwNDIwMDg1MTE2WhcNMjEw"
									"NDE3MDg1MTE2WjCBgDExMC8GA1UEAwwoa2FybHlyaWNlZGl0b3IuY29tLmNlcnRp"
									"ZmljYXRlLmF1dGhvcml0eTETMBEGA1UECAwKQ2FsaWZvcm5pYTELMAkGA1UEBhMC"
									"VVMxKTAnBgkqhkiG9w0BCQEWGnN1cHBvcnRAa2FybHlyaWNlZGl0b3IuY29tMIGf"
									"MA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQCv6WOtHgsndu8IaZyP2xgke0rHAWJv"
									"y5cPpRNWNB/2G5ogbL629A9a3ehVIRpAbJyHHiSuNX+wc4YiczwxZjW32KU3QFKf"
									"XtCPDOVX5OdToMnKIEngSD65QiYSv/RqCW45z+Mc0LqWAE9BftEybpdUfubYV5pY"
									"r6pckkKWpPb1xQIDAQABoxAwDjAMBgNVHRMEBTADAQH/MA0GCSqGSIb3DQEBBQUA"
									"A4GBAC08oP+QzbT8TfQUqXTnAxSQNVWZAPHL4wOQMbC/MDumdg2N8iYDPFEX3QKE"
									"vfWiqr3nwmOrAZxAQh6iqMa73JqaVm6h2oqrAcf9XnaoR63G+gk1EluDD9AK8MKf"
									"b3IE9nt1TDhlJkbGG8rMD692HkpMGggsco93PQLWmtvcyIIo";

static const char * CA_SUBJECT = "karlyriceditor.com.certificate.authority";


class LicensingPrivate
{
	public:
		bool	m_valid;
		QString	m_errmsg;
		QString	m_subject;
		QDate	m_expires;
};



Licensing::Licensing()
{
	d = new LicensingPrivate();
	d->m_valid = false;
}

Licensing::~Licensing()
{
	delete d;
}

bool Licensing::isEnabled() const
{
#if defined (USE_LICENSING)
	return true;
#else
	return false;
#endif
}


QString	Licensing::subject() const
{
	return d->m_valid ? d->m_subject : QString();
}

QDate Licensing::expires() const
{
	return d->m_valid ? d->m_expires : QDate();
}

bool Licensing::init()
{
#if defined (USE_LICENSING)
	OpenSSL_add_all_algorithms();
	return true;
#else
	return false;
#endif
}


bool Licensing::validate( const QString& license )
{
#if defined (USE_LICENSING)
	char subject[1024];
	ASN1_TIME *naft;
	X509 * cert = 0;
	X509 * cacert = 0;
	X509_STORE_CTX * storeContext = 0;
	X509_STORE * certStore = 0;

	// Reset the license
	d->m_valid = false;

	// Get the DER cert data
	QByteArray certdata = QByteArray::fromBase64( license.toUtf8() );

	if ( certdata.isEmpty() )
	{
		d->m_errmsg = "Invalid encoded license content";
		return false;
	}

	// Convert the DER-encoded certificate to an X509 object
	long len = certdata.size();
	const unsigned char * pcertdata = (const unsigned char *) certdata.data();
	cert = d2i_X509( 0, &pcertdata, len );

	if ( !cert )
	{
		d->m_errmsg = "Invalid license content (error 2010)";
		goto cleanup;
	}

	// Create a cert store
	certStore = X509_STORE_new();

	if ( !certStore )
	{
		d->m_errmsg = "Cannot allocate memory";
		goto cleanup;
	}

	if ( X509_STORE_add_cert( certStore, cert ) <= 0 )
	{
		d->m_errmsg = "Invalid license content (error 2012)";
		goto cleanup;
	}

	// Get the DER cert data for CA
	certdata = QByteArray::fromBase64( QByteArray(CA_DER_CERT) );

	if ( certdata.isEmpty() )
	{
		d->m_errmsg = "Invalid internal CA certificate (error 2020)";
		goto cleanup;
	}

	// Convert the DER-encoded CA certificate to an X509 object
	len = certdata.size();
	pcertdata = (const unsigned char *) certdata.data();
	cacert = d2i_X509( 0, &pcertdata, len );

	if ( !cacert )
	{
		d->m_errmsg = "Invalid internal CA content (error 2025)";
		goto cleanup;
	}

	// Add the CA cert to the store
	if ( X509_STORE_add_cert( certStore, cacert ) <= 0 )
	{
		d->m_errmsg = "Invalid license content (error 2030)";
		goto cleanup;
	}

	storeContext = X509_STORE_CTX_new();

	if ( !storeContext )
	{
		d->m_errmsg = "Cannot allocate memory";
		goto cleanup;
	}

	// Prepare the chain
	if ( X509_STORE_CTX_init( storeContext, certStore, cert, NULL ) <= 0 )
	{
		d->m_errmsg = "Cannot prepare cert chain";
		goto cleanup;
	}

	if ( X509_verify_cert( storeContext ) <= 0 )
	{
		int err = X509_STORE_CTX_get_error( storeContext );
		switch ( err )
		{
			case X509_V_ERR_CERT_NOT_YET_VALID:
				d->m_errmsg = "License is not yet valid";
				break;

			case X509_V_ERR_CERT_HAS_EXPIRED:
				d->m_errmsg = "License expired";
				break;

			default:
				d->m_errmsg = QString("License verification error (error %1)") .arg( err );
				break;
		}

		goto cleanup;
	}

	// Cert is valid, check the issuer
	if ( X509_NAME_get_text_by_NID( X509_get_issuer_name( cert ), NID_commonName, subject, sizeof(subject) ) < 1 )
	{
		d->m_errmsg = "Cannot parse the license issuer";
		goto cleanup;
	}

	if ( strcmp( subject, CA_SUBJECT) )
	{
		d->m_errmsg = "License is not issued by the valid issuer";
		goto cleanup;
	}

	// Parse the CN
	if ( X509_NAME_get_text_by_NID( X509_get_subject_name( cert ), NID_commonName, subject, sizeof(subject) ) < 1 )
	{
		d->m_errmsg = "Cannot parse the license subject";
		goto cleanup;
	}

	// Our CA public cert is also valid as it is self-signed, so we need to check CNAME!
	if ( !strcmp( subject, CA_SUBJECT) )
	{
		d->m_errmsg = "Cannot use the CA certificate as the license";
		goto cleanup;
	}

	d->m_subject = subject;

	// Parse the expiration date
	naft = X509_get_notAfter( cert );

	// We're not interested in time
	int year, month, day;

	if ( naft->type == V_ASN1_UTCTIME
	&& sscanf( (const char *) naft->data, "%02d%02d%02d", &year, &month, &day ) == 3 )
	{
		// Fix the year
		year += 2000;

		d->m_expires.setDate( year, month, day );
		d->m_valid = true;
	}
	else if ( naft->type == V_ASN1_GENERALIZEDTIME
	&& sscanf( (const char *) naft->data, "%04d%02d%02d", &year, &month, &day ) != 3 )
	{
		d->m_expires.setDate( year, month, day );
		d->m_valid = true;
	}

cleanup:
	if ( cert )
		X509_free( cert );

	if ( cacert )
		X509_free( cacert );

	if ( storeContext )
		X509_STORE_CTX_free( storeContext );

	if ( certStore )
		X509_STORE_free( certStore );

	return d->m_valid;

#else
	return false;
#endif
}

bool Licensing::isValid() const
{
	return d->m_valid;
}

QString Licensing::errMsg() const
{
	return d->m_errmsg;
}
