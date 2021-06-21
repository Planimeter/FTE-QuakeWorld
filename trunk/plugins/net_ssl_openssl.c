#include "plugin.h"
#include "netinc.h"

plugfsfuncs_t *fsfuncs;

#undef SHA1
#undef HMAC
#include "openssl/bio.h"
#include "openssl/ssl.h"
#include "openssl/err.h"
#include "openssl/conf.h"

#define assert(c) {if (!(c)) Con_Printf("assert failed: "STRINGIFY(c)"\n");}

static qboolean OSSL_Init(void);
static int ossl_fte_certctx;
struct fte_certctx_s
{
	const char *peername;
	qboolean dtls;
};

static struct
{
	X509 *servercert;
	EVP_PKEY *privatekey;
} vhost;

static BIO_METHOD *biometh_vfs;

static int OSSL_Bio_FWrite(BIO *h, const char *buf, int size)
{
	vfsfile_t *f = BIO_get_data(h);
	int r = VFS_WRITE(f, buf, size);
	BIO_clear_retry_flags(h);
	if (r == 0)
	{
		BIO_set_retry_write(h);
		r = -1; //paranoia
	}
	return r;
}
static int OSSL_Bio_FRead(BIO *h, char *buf, int size)
{
	vfsfile_t *f = BIO_get_data(h);
	int r = VFS_READ(f, buf, size);
	BIO_clear_retry_flags(h);
	if (r == 0)	//no data yet.
	{
		BIO_set_retry_read(h);
		r = -1;	//shouldn't be needed, but I'm paranoid
	}
	return r;
}
static int OSSL_Bio_FPuts(BIO *h, const char *buf)
{
	return OSSL_Bio_FWrite(h, buf, strlen(buf));
}
static long OSSL_Bio_FCtrl(BIO *h, int cmd, long arg1, void *arg2)
{
	vfsfile_t *f = BIO_get_data(h);
	switch(cmd)
	{
	case BIO_CTRL_FLUSH:
		VFS_FLUSH(f);
		return 1;
	default:
		Con_Printf("OSSL_Bio_FCtrl: unknown cmd %i\n", cmd);
	case BIO_CTRL_PUSH:
	case BIO_CTRL_POP:
		return 0;
	}
	return 0;	//failure
}
static long OSSL_Bio_FOtherCtrl(BIO *h, int cmd, BIO_info_cb *cb)
{
	switch(cmd)
	{
	default:
		Con_Printf("OSSL_Bio_FOtherCtrl unknown cmd %i\n", cmd);
		return 0;
	}
	return 0;	//failure
}
static int OSSL_Bio_FCreate(BIO *h)
{	//we'll have to fill this in after we create the bio.
	BIO_set_data(h, NULL);
	return 1;
}
static int OSSL_Bio_FDestroy(BIO *h)
{
	vfsfile_t *f = BIO_get_data(h);
	VFS_CLOSE(f);
	BIO_set_data(h, NULL);
	return 1;
}

static int OSSL_PrintError_CB (const char *str, size_t len, void *u)
{
	Con_Printf("%s\n", str);
	return 1;
}

typedef struct
{
	vfsfile_t funcs;
	struct fte_certctx_s cert;
	SSL_CTX *ctx;
	BIO *bio;
	SSL *ssl;
} osslvfs_t;
static int QDECL OSSL_FRead (struct vfsfile_s *file, void *buffer, int bytestoread)
{
	osslvfs_t *o = (osslvfs_t*)file;
	int r = BIO_read(o->bio, buffer, bytestoread);
	if (r <= 0)
	{
		if (BIO_should_io_special(o->bio))
		{
			switch(BIO_get_retry_reason(o->bio))
			{
			//these are temporary errors, try again later.
			case BIO_RR_SSL_X509_LOOKUP:
				return -1;	//certificate failure.
			case BIO_RR_ACCEPT:
			case BIO_RR_CONNECT:
				return -1;	//should never happen
			};
		}
		if (BIO_should_retry(o->bio))
			return 0;
		return -1;	//eof or something
	}
	return r;
}
static int QDECL OSSL_FWrite (struct vfsfile_s *file, const void *buffer, int bytestowrite)
{
	osslvfs_t *o = (osslvfs_t*)file;
	int r = BIO_write(o->bio, buffer, bytestowrite);
	if (r <= 0)
	{
		if (BIO_should_io_special(o->bio))
		{
			switch(BIO_get_retry_reason(o->bio))
			{
			//these are temporary errors, try again later.
			case BIO_RR_SSL_X509_LOOKUP:
				return -1;	//certificate failure.
			case BIO_RR_ACCEPT:
			case BIO_RR_CONNECT:
				return -1;	//should never happen
			}
		}
		if (BIO_should_retry(o->bio))
			return 0;
		return -1;	//eof or something
	}
	return r;
}
//static qboolean QDECL OSSL_Seek (struct vfsfile_s *file, qofs_t pos);	//returns false for error
//static qofs_t QDECL OSSL_Tell (struct vfsfile_s *file);
//static qofs_t QDECL OSSL_GetLen (struct vfsfile_s *file);	//could give some lag
static qboolean QDECL OSSL_Close (struct vfsfile_s *file)
{
	osslvfs_t *o = (osslvfs_t*)file;
	BIO_free(o->bio);
	SSL_CTX_free(o->ctx);
	free(o);
	return true;	//success, I guess
}
//static void QDECL OSSL_Flush (struct vfsfile_s *file);



static qboolean print_cn_name(X509_NAME* const name, const char *utf8match, const char *prefix)
{
    int idx = -1;
	qboolean success = 0;
    unsigned char *utf8 = NULL;
	X509_NAME_ENTRY* entry;
	ASN1_STRING* data;
	int length;

    do
    {
        if(!name) break; /* failed */

        idx = X509_NAME_get_index_by_NID(name, NID_commonName, -1);
        if(!(idx > -1))  break; /* failed */

        entry = X509_NAME_get_entry(name, idx);
        if(!entry) break; /* failed */

        data = X509_NAME_ENTRY_get_data(entry);
        if(!data) break; /* failed */

        length = ASN1_STRING_to_UTF8(&utf8, data);
        if(!utf8 || !(length > 0))  break; /* failed */

		if (utf8match)
			success = !strcmp(utf8, utf8match);
		else
		{
			Con_Printf("%s%s", prefix, utf8);
			success = 1;
		}

    } while (0);

    if(utf8)
        OPENSSL_free(utf8);

	return success;
}

static int OSSL_Verify_Peer(int preverify_ok, X509_STORE_CTX *x509_ctx)
{
	SSL *ssl = X509_STORE_CTX_get_ex_data(x509_ctx, SSL_get_ex_data_X509_STORE_CTX_idx());
	struct fte_certctx_s *uctx = SSL_get_ex_data(ssl, ossl_fte_certctx);

	if(preverify_ok == 0)
	{
		int depth = X509_STORE_CTX_get_error_depth(x509_ctx);
		int err = X509_STORE_CTX_get_error(x509_ctx);

		X509* cert = X509_STORE_CTX_get_current_cert(x509_ctx);
		X509_NAME* iname = cert ? X509_get_issuer_name(cert) : NULL;
		//X509_NAME* sname = cert ? X509_get_subject_name(cert) : NULL;

		if (err == X509_V_ERR_CERT_HAS_EXPIRED || err == X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT)
		{
			size_t knownsize;
			qbyte *knowndata = TLS_GetKnownCertificate(uctx->peername, &knownsize);
			if (knowndata)
			{	//check
				size_t blobsize;
				qbyte *blob;
				qbyte *end;
				blobsize = i2d_X509(cert, NULL);
				if (blobsize != knownsize)
					return 0;	//fail if the size doesn't match.
				blob = alloca(blobsize);
				end = blob;
				i2d_X509(cert, &end);
				if (memcmp(blob, knowndata, blobsize))
					return 0;
				return 1;	//exact match to a known cert. yay. allow it.
			}

#ifdef HAVE_CLIENT
			if (uctx->dtls)
			{
				unsigned int probs = 0;
				size_t blobsize;
				qbyte *blob;
				qbyte *end;
				blobsize = i2d_X509(cert, NULL);
				blob = alloca(blobsize);
				end = blob;
				i2d_X509(cert, &end);

				switch(err)
				{
				case 0:
					probs |= CERTLOG_WRONGHOST;
					break;
				case X509_V_ERR_CERT_HAS_EXPIRED:
					probs |= CERTLOG_EXPIRED;
					break;
				case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
					probs |= CERTLOG_MISSINGCA;
					break;
				default:
					probs |= CERTLOG_UNKNOWN;
					break;
				}
				if (CertLog_ConnectOkay(uctx->peername, blob, blobsize, probs))
					return 1; //ignore the errors...
			}
#endif
		}

		Con_Printf(CON_ERROR"%s ", uctx->peername);
		//FIXME: this is probably on a worker thread. expect munged prints.
		//print_cn_name(sname, NULL, CON_ERROR);
		Con_Printf(" (issued by ");
		print_cn_name(iname, NULL, S_COLOR_YELLOW);
		Con_Printf(")");
		if(depth == 0) {
			/* If depth is 0, its the server's certificate. Print the SANs too */
	//		print_san_name("Subject (san)", cert);
		}

		if(err == X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY)
			Con_Printf(":  Error = X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY\n");
		else if (err == X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE)
			Con_Printf(":  Error = X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE\n");
		else if(err == X509_V_ERR_CERT_UNTRUSTED)
			Con_Printf(":  Error = X509_V_ERR_CERT_UNTRUSTED\n");
		else if(err == X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN)
			Con_Printf(":  Error = X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN\n");
		else if(err == X509_V_ERR_CERT_NOT_YET_VALID)
			Con_Printf(":  Error = X509_V_ERR_CERT_NOT_YET_VALID\n");
		else if(err == X509_V_ERR_CERT_HAS_EXPIRED)
			Con_Printf(":  Error = X509_V_ERR_CERT_HAS_EXPIRED\n");
		else if(err == X509_V_OK)
			Con_Printf(":  Error = X509_V_OK\n");
		else if(err == X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT)
			Con_Printf(":  Error = X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT\n");
		else
			Con_Printf(":  Error = %d\n", err);
	}

	if (!preverify_ok && cvarfuncs->GetFloat("tls_ignorecertificateerrors"))
	{
		Con_Printf(CON_ERROR "%s: Ignoring certificate errors (tls_ignorecertificateerrors is set)\n", uctx->peername);
		return 1;
	}

	return preverify_ok;
}


static vfsfile_t *OSSL_OpenPrivKeyFile(char *nativename, size_t nativesize)
{
#define privname "privkey.pem"
	vfsfile_t *privf;
	const char *mode = nativename?"wb":"rb";
	/*int i = COM_CheckParm("-privkey");
	if (i++)
	{
		if (nativename)
			Q_strncpyz(nativename, com_argv[i], nativesize);
		privf = FS_OpenVFS(com_argv[i], mode, FS_SYSTEM);
	}
	else*/
	{
		if (nativename)
			if (!fsfuncs->NativePath(privname, FS_ROOT, nativename, nativesize))
				return NULL;

		privf = fsfuncs->OpenVFS(privname, mode, FS_ROOT);
	}
	return privf;
#undef privname
}
static vfsfile_t *OSSL_OpenPubKeyFile(char *nativename, size_t nativesize)
{
#define fullchainname "fullchain.pem"
#define pubname "cert.pem"
	vfsfile_t *pubf = NULL;
	const char *mode = nativename?"wb":"rb";
	/*int i = COM_CheckParm("-pubkey");
	if (i++)
	{
		if (nativename)
			Q_strncpyz(nativename, com_argv[i], nativesize);
		pubf = FS_OpenVFS(com_argv[i], mode, FS_SYSTEM);
	}
	else*/
	{
		if (!pubf && (!nativename || fsfuncs->NativePath(fullchainname, FS_ROOT, nativename, nativesize)))
			pubf = fsfuncs->OpenVFS(fullchainname, mode, FS_ROOT);
		if (!pubf && (!nativename || fsfuncs->NativePath(pubname, FS_ROOT, nativename, nativesize)))
			pubf = fsfuncs->OpenVFS(pubname, mode, FS_ROOT);
	}
	return pubf;
#undef pubname
}
static BIO *OSSL_BioFromFile(vfsfile_t *f)
{
	qbyte buf[4096];
	int r;
	BIO *b = BIO_new(BIO_s_mem());
	if (f)
	{
		for(;;)
		{
			r = VFS_READ(f, buf, sizeof(buf));
			if (r <= 0)
				break;
			BIO_write(b, buf, r);
		}
		VFS_CLOSE(f);
	}

	return b;
}
static void OSSL_OpenPrivKey(void)
{
	BIO *bio = OSSL_BioFromFile(OSSL_OpenPrivKeyFile(NULL,0));
	vhost.privatekey = PEM_read_bio_PrivateKey(bio, NULL, NULL, NULL);
	BIO_free(bio);
}
static void OSSL_OpenPubKey(void)
{
	BIO *bio = OSSL_BioFromFile(OSSL_OpenPubKeyFile(NULL,0));
	vhost.servercert = PEM_read_bio_X509(bio, NULL, NULL, NULL);
	BIO_free(bio);
}

static vfsfile_t *OSSL_OpenVFS(const char *hostname, vfsfile_t *source, qboolean isserver)
{
	BIO *sink;
	osslvfs_t *n;
	if (!OSSL_Init())
		return NULL;	//FAIL!

	n = calloc(sizeof(*n) + strlen(hostname)+1, 1);

	n->funcs.ReadBytes = OSSL_FRead;
	n->funcs.WriteBytes = OSSL_FWrite;
	n->funcs.Seek = NULL;
	n->funcs.Tell = NULL;
	n->funcs.GetLen = NULL;
	n->funcs.Close = OSSL_Close;
	n->funcs.Flush = NULL;
	n->funcs.seekstyle = SS_UNSEEKABLE;

	n->cert.peername = strcpy((char*)(n+1), hostname);
	n->cert.dtls = false;

	ERR_print_errors_cb(OSSL_PrintError_CB, NULL);

	sink = BIO_new(biometh_vfs);
	if (sink)
	{
		n->ctx = SSL_CTX_new(isserver?TLS_server_method():TLS_client_method());
		if (n->ctx)
		{
			assert(1==SSL_CTX_set_cipher_list(n->ctx, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH"));

			SSL_CTX_set_session_cache_mode(n->ctx, SSL_SESS_CACHE_OFF);

			SSL_CTX_set_default_verify_paths(n->ctx);
			SSL_CTX_set_verify(n->ctx, SSL_VERIFY_PEER, OSSL_Verify_Peer);
			SSL_CTX_set_verify_depth(n->ctx, 5);
			SSL_CTX_set_options(n->ctx, SSL_OP_NO_COMPRESSION);	//compression allows guessing the contents of the stream somehow.

			if (isserver)
			{
				if (vhost.servercert && vhost.privatekey)
				{
					SSL_CTX_use_certificate(n->ctx, vhost.servercert);
					SSL_CTX_use_PrivateKey(n->ctx, vhost.privatekey);
					assert(1==SSL_CTX_check_private_key(n->ctx));
				}
			}

			n->bio = BIO_new_ssl(n->ctx, !isserver);

			//set up the source/sink
			BIO_set_data(sink, source);	//source now belongs to the bio
			BIO_set_init(sink, true);	//our sink is now ready...
			n->bio = BIO_push(n->bio, sink);
			BIO_free(sink);
			sink = NULL;

			BIO_get_ssl(n->bio, &n->ssl);
			SSL_set_ex_data(n->ssl, ossl_fte_certctx, &n->cert);
			SSL_set_mode(n->ssl, SSL_MODE_ENABLE_PARTIAL_WRITE|SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
			SSL_set_tlsext_host_name(n->ssl, n->cert.peername);	//let the server know which cert to send
			BIO_do_connect(n->bio);
			return &n->funcs;
		}
		BIO_free(sink);
	}
	return NULL;
}
/*static int OSSL_GetChannelBinding(vfsfile_t *vf, qbyte *binddata, size_t *bindsize)
{
	//FIXME: not yet supported. tbh I've no idea how to get that data. probably something convoluted.
	return -1;
}*/



static BIO_METHOD *biometh_dtls;
typedef struct {
	struct fte_certctx_s cert;

	void *cbctx;
	neterr_t(*push)(void *cbctx, const qbyte *data, size_t datasize);

	SSL_CTX *ctx;
	BIO *bio;
	SSL *ssl;

//	BIO *sink;
	qbyte *pending;
	size_t pendingsize;
} ossldtls_t;
static int OSSL_Bio_DWrite(BIO *h, const char *buf, int size)
{
	ossldtls_t *f = BIO_get_data(h);
	neterr_t r = f->push(f->cbctx, buf, size);

	BIO_clear_retry_flags(h);
	switch(r)
	{
	case NETERR_SENT:
		return size;
	case NETERR_NOROUTE:
	case NETERR_DISCONNECTED:
		return -1;
	case NETERR_MTU:
		return -1;
	case NETERR_CLOGGED:
		BIO_set_retry_write(h);
		return -1;
	}
	return r;
}
static int OSSL_Bio_DRead(BIO *h, char *buf, int size)
{
	ossldtls_t *f = BIO_get_data(h);

	BIO_clear_retry_flags(h);
	if (f->pending)
	{
		size = min(size, f->pendingsize);
		memcpy(buf, f->pending, f->pendingsize);

		//we've read it now, don't read it again.
		f->pending = 0;
		f->pendingsize = 0;
		return size;
	}
	//nothing available.
	BIO_set_retry_read(h);
	return -1;
}
static long OSSL_Bio_DCtrl(BIO *h, int cmd, long arg1, void *arg2)
{
//	ossldtls_t *f = BIO_get_data(h);
	switch(cmd)
	{
	case BIO_CTRL_FLUSH:
		return 1;


	case BIO_CTRL_DGRAM_SET_NEXT_TIMEOUT:	//we're non-blocking, so this doesn't affect us.
	case BIO_CTRL_DGRAM_GET_MTU_OVERHEAD:
	case BIO_CTRL_WPENDING:
	case BIO_CTRL_DGRAM_QUERY_MTU:
	case BIO_CTRL_DGRAM_SET_MTU:
		return 0;


	default:
		Con_Printf("OSSL_Bio_DCtrl: unknown cmd %i\n", cmd);
	case BIO_CTRL_PUSH:
	case BIO_CTRL_POP:
		return 0;
	}
	return 0;	//failure
}
static long OSSL_Bio_DOtherCtrl(BIO *h, int cmd, BIO_info_cb *cb)
{
	switch(cmd)
	{
	default:
		Con_Printf("OSSL_Bio_DOtherCtrl unknown cmd %i\n", cmd);
		return 0;
	}
	return 0;	//failure
}
static int OSSL_Bio_DCreate(BIO *h)
{	//we'll have to fill this in after we create the bio.
	BIO_set_data(h, NULL);
	return 1;
}
static int OSSL_Bio_DDestroy(BIO *h)
{
	BIO_set_data(h, NULL);
	return 1;
}

static void *OSSL_CreateContext(const char *remotehost, void *cbctx, neterr_t(*push)(void *cbctx, const qbyte *data, size_t datasize), qboolean isserver)
{	//if remotehost is null then their certificate will not be validated.
	ossldtls_t *n = calloc(sizeof(*n) + strlen(remotehost)+1, 1);
	BIO *sink;

	n->cbctx = cbctx;
	n->push = push;

	n->ctx = SSL_CTX_new(isserver?DTLS_server_method():DTLS_client_method());

	n->cert.peername = strcpy((char*)(n+1), remotehost);
	n->cert.dtls = true;

	if (n->ctx)
	{
		assert(1==SSL_CTX_set_cipher_list(n->ctx, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH"));

		SSL_CTX_set_session_cache_mode(n->ctx, SSL_SESS_CACHE_OFF);

		SSL_CTX_set_verify(n->ctx, SSL_VERIFY_PEER, OSSL_Verify_Peer);
		SSL_CTX_set_verify_depth(n->ctx, 5);
		SSL_CTX_set_options(n->ctx, SSL_OP_NO_COMPRESSION);	//compression allows guessing the contents of the stream somehow.

		//SSL_CTX_use_certificate_file
		//FIXME: SSL_CTX_use_certificate_file aka SSL_CTX_use_certificate(PEM_read_bio_X509)
		//FIXME: SSL_CTX_use_PrivateKey_file aka SSL_CTX_use_PrivateKey(PEM_read_bio_PrivateKey)
		//assert(1==SSL_CTX_check_private_key(n->ctx));

		{
			n->bio = BIO_new_ssl(n->ctx, !isserver);

			//set up the source/sink
			sink = BIO_new(biometh_dtls);
			if (sink)
			{
				BIO_set_data(sink, n);
				BIO_set_init(sink, true);	//our sink is now ready...
				n->bio = BIO_push(n->bio, sink);
				BIO_free(sink);
				sink = NULL;
			}

			BIO_get_ssl(n->bio, &n->ssl);
			SSL_set_ex_data(n->ssl, ossl_fte_certctx, &n->cert);
			SSL_set_tlsext_host_name(n->ssl, remotehost);	//let the server know which cert to send
			BIO_do_connect(n->bio);
			ERR_print_errors_cb(OSSL_PrintError_CB, NULL);
			return n;
		}
	}

	return NULL;
}
static void OSSL_DestroyContext(void *ctx)
{
	ossldtls_t *o = (ossldtls_t*)ctx;
	BIO_free(o->bio);
	SSL_CTX_free(o->ctx);
	free(o);
}
static neterr_t OSSL_Transmit(void *ctx, const qbyte *data, size_t datasize)
{	//we're sending data
	ossldtls_t *o = (ossldtls_t*)ctx;
	int r = BIO_write(o->bio, data, datasize);
	if (r <= 0)
	{
		if (BIO_should_io_special(o->bio))
		{
			switch(BIO_get_retry_reason(o->bio))
			{
			//these are temporary errors, try again later.
			case BIO_RR_SSL_X509_LOOKUP:
				return NETERR_NOROUTE;	//certificate failure.
			case BIO_RR_ACCEPT:
			case BIO_RR_CONNECT:
				return NETERR_NOROUTE;	//should never happen
			}
		}
		if (BIO_should_retry(o->bio))
			return 0;
		return NETERR_NOROUTE;	//eof or something
	}
	return NETERR_SENT;
}
static neterr_t OSSL_Received(void *ctx, sizebuf_t *message)
{	//we have received some encrypted data...
	ossldtls_t *o = (ossldtls_t*)ctx;
	int r;

	o->pending = message->data;
	o->pendingsize = message->cursize;
	r = BIO_read(o->bio, message->data, message->maxsize);
	o->pending = NULL;
	o->pendingsize = 0;

	if (r > 0)
	{
		message->cursize = r;
		return NETERR_SENT;
	}
	else
	{
		if (BIO_should_io_special(o->bio))
		{
			switch(BIO_get_retry_reason(o->bio))
			{
			//these are temporary errors, try again later.
			case BIO_RR_SSL_X509_LOOKUP:
				return NETERR_NOROUTE;	//certificate failure.
			case BIO_RR_ACCEPT:
			case BIO_RR_CONNECT:
				return NETERR_NOROUTE;	//should never happen
			}
		}
		if (BIO_should_retry(o->bio))
			return 0;
		return NETERR_NOROUTE;	//eof or something
	}
	return NETERR_NOROUTE;
}
static neterr_t OSSL_Timeouts(void *ctx)
{	//keep it ticking over, or something.
	return OSSL_Received(ctx, NULL);
}

static dtlsfuncs_t ossl_dtlsfuncs =
{
	OSSL_CreateContext,
	OSSL_DestroyContext,
	OSSL_Transmit,
	OSSL_Received,
	OSSL_Timeouts
};
static const dtlsfuncs_t *OSSL_InitClient(void)
{
	if (OSSL_Init())
		return &ossl_dtlsfuncs;
	return NULL;
}
static const dtlsfuncs_t *OSSL_InitServer(void)
{
	if (OSSL_Init())
		return &ossl_dtlsfuncs;
	return NULL;
}











static qboolean OSSL_Init(void)
{
	static qboolean inited;
	static qboolean init_success;
	if (inited)
		return init_success;
#ifdef LOADERTHREAD
	Sys_LockMutex(com_resourcemutex);
	if (inited)	//now check again, just in case
	{
		Sys_UnlockMutex(com_resourcemutex);
		return init_success;
	}
#endif

	SSL_library_init();
    SSL_load_error_strings();
    ERR_load_BIO_strings();
//	OPENSSL_config(NULL);
	ERR_print_errors_cb(OSSL_PrintError_CB, NULL);

	OSSL_OpenPubKey();
	OSSL_OpenPrivKey();

	biometh_vfs = BIO_meth_new(BIO_get_new_index()|BIO_TYPE_SOURCE_SINK|BIO_TYPE_DESCRIPTOR, "fte_vfs");
	if (biometh_vfs)
	{
		BIO_meth_set_write(biometh_vfs, OSSL_Bio_FWrite);
		BIO_meth_set_read(biometh_vfs, OSSL_Bio_FRead);
		BIO_meth_set_puts(biometh_vfs, OSSL_Bio_FPuts);	//I cannot see how gets/puts can work with dtls...
//		BIO_meth_set_gets(biometh_vfs, OSSL_Bio_FGets);
		BIO_meth_set_ctrl(biometh_vfs, OSSL_Bio_FCtrl);
		BIO_meth_set_create(biometh_vfs, OSSL_Bio_FCreate);
		BIO_meth_set_destroy(biometh_vfs, OSSL_Bio_FDestroy);
		BIO_meth_set_callback_ctrl(biometh_vfs, OSSL_Bio_FOtherCtrl);
		init_success |= 1;
	}

	biometh_dtls = BIO_meth_new(BIO_get_new_index()|BIO_TYPE_SOURCE_SINK|BIO_TYPE_DESCRIPTOR, "fte_dtls");
	if (biometh_dtls)
	{
		BIO_meth_set_write(biometh_dtls, OSSL_Bio_DWrite);
		BIO_meth_set_read(biometh_dtls, OSSL_Bio_DRead);
//		BIO_meth_set_puts(biometh_dtls, OSSL_Bio_DPuts);	//I cannot see how gets/puts can work with dtls...
//		BIO_meth_set_gets(biometh_dtls, OSSL_Bio_DGets);
		BIO_meth_set_ctrl(biometh_dtls, OSSL_Bio_DCtrl);
		BIO_meth_set_create(biometh_dtls, OSSL_Bio_DCreate);
		BIO_meth_set_destroy(biometh_dtls, OSSL_Bio_DDestroy);
		BIO_meth_set_callback_ctrl(biometh_dtls, OSSL_Bio_DOtherCtrl);
		init_success |= 2;
	}

	ossl_fte_certctx = SSL_get_ex_new_index(0, "ossl_fte_certctx", NULL, NULL, NULL);

	inited = true;
#ifdef LOADERTHREAD
	Sys_UnlockMutex(com_resourcemutex);
#endif
	return init_success;
}

static enum hashvalidation_e OSSL_VerifyHash(const qbyte *hashdata, size_t hashsize, const qbyte *pubkeydata, size_t pubkeysize, const qbyte *signdata, size_t signsize)
{
	int result = VH_UNSUPPORTED;
	BIO *bio_pubkey = BIO_new_mem_buf(pubkeydata, pubkeysize);
	if (bio_pubkey)
	{
		X509 *x509_pubkey = PEM_read_bio_X509(bio_pubkey, NULL, NULL, NULL);
		if (x509_pubkey)
		{
			EVP_PKEY *evp_pubkey = X509_get_pubkey(x509_pubkey);
			if (evp_pubkey)
			{
				RSA *rsa_pubkey = EVP_PKEY_get1_RSA(evp_pubkey);
				if (rsa_pubkey)
				{
					if (1 == RSA_verify(NID_sha512, hashdata, hashsize,	signdata, signsize, rsa_pubkey))
						result = VH_CORRECT;
					else
						result = VH_INCORRECT;
					RSA_free(rsa_pubkey);
				}
				EVP_PKEY_free(evp_pubkey);
			}
			X509_free(x509_pubkey);
		}
		BIO_free(bio_pubkey);
	}

	return result;
}

static ftecrypto_t crypto_openssl =
{
	"OpenSSL",
	OSSL_OpenVFS,
	NULL,//OSSL_GetChannelBinding,
	OSSL_InitClient,
	OSSL_InitServer,
	OSSL_VerifyHash,
	NULL,
};

void *TLS_GetKnownCertificate(const char *certname, size_t *size){return NULL;}
qboolean CertLog_ConnectOkay(const char *hostname, void *cert, size_t certsize, unsigned int certlogproblems){return false;}
qboolean Plug_Init(void)
{
	fsfuncs = plugfuncs->GetEngineInterface(plugfsfuncs_name, sizeof(*fsfuncs));
	if (!fsfuncs)
		return false;
	return plugfuncs->ExportInterface("Crypto", &crypto_openssl, sizeof(crypto_openssl)); //export a named interface struct to the engine
}