
#include <uv.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

/**
* Our tcp server object.
*/
uv_tcp_t server;

/**
* Shared reference to our event loop.
*/
uv_loop_t * loop;

/**
* Allocates a buffer which we can use for reading.
*/
static void alloc_cb(uv_handle_t* handle,
					 size_t suggested_size,
					 uv_buf_t* buf) {
						 static char slab[65536];
						 assert(suggested_size <= sizeof(slab));
						 buf->base = slab;
						 buf->len = sizeof(slab);
}

void connection_cb(uv_stream_t * server, int status);
void read_cb(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);

int main() {
	loop = uv_default_loop();

	/* convert a humanreadable ip address to a c struct */
	struct sockaddr_in bind_addr ;
	int ret = uv_ip4_addr("0.0.0.0", 8430, &bind_addr);    

	/* initialize the server */
	uv_tcp_init(loop, &server);
	/* bind the server to the address above */
	uv_tcp_bind(&server, (const struct sockaddr *)&bind_addr);

	/* let the server listen on the address for new connections */
	int r = uv_listen((uv_stream_t *) &server, 128, connection_cb);

	if (r) {
		return fprintf(stderr, "Error on listening: %s.\n", 
			uv_strerror(r));
	}

	/* execute all tasks in queue */
	return uv_run(loop, UV_RUN_DEFAULT);
}

/**
* Callback which is executed on each new connection.
*/
void connection_cb(uv_stream_t * server, int status) {
	/* dynamically allocate a new client stream object on conn */
	uv_tcp_t * client = (uv_tcp_t *)malloc(sizeof(uv_tcp_t));

	/* if status not zero there was an error */
	if (status) {
		fprintf(stderr, "Error on listening: %s.\n", 
			uv_strerror(status));
	}

	/* initialize the new client */
	uv_tcp_init(loop, client);

	/* now let bind the client to the server to be used for incomings */
	if (uv_accept(server, (uv_stream_t *) client) == 0) {
		/* start reading from stream */
		int r = uv_read_start((uv_stream_t *) client, alloc_cb, read_cb);

		if (r < 0) {
			fprintf(stderr, "Error on reading client stream: %s.\n", 
				uv_strerror(r));
		}
	} else {
		/* close client stream on error */
		uv_close((uv_handle_t *) client, NULL);
	}
}

/**
* Callback which is executed on each readable state.
*/
void read_cb(uv_stream_t * stream, ssize_t nread,const uv_buf_t *buf) {
	/* dynamically allocate memory for a new write task */
	uv_write_t * req = (uv_write_t *) malloc(sizeof(uv_write_t));

	/* if read bytes counter -1 there is an error or EOF */
	if (nread >= 0 ){
		/* write sync the incoming buffer to the socket */
		//const char *g_safebox_buf =
		//	"<?xml version=\"1.0\"?>"
		//	"<!DOCTYPE cross-domain-policy SYSTEM \"http://www.macromedia.com/xml/dtds/cross-domain-policy.dtd\">"
		//	"<cross-domain-policy>"
		//	"   <allow-access-from domain=\"*\" to-ports=\"*\" />"
		//	"</cross-domain-policy>";
		//const int g_safebox_buf_size = strlen(g_safebox_buf)+1;
		//((uv_buf_t *)buf)->base = (char*)g_safebox_buf;
		//((uv_buf_t *)buf)->len = g_safebox_buf_size;

		int r = uv_write(req, stream, buf, 1, NULL);

		if (r) {
			fprintf(stderr, "Error on writing client stream: %s.\n", 
				uv_strerror(r));
		}

		/* free the remaining memory */
		//free(buf.base);
	}
	else {
		if (nread != UV_EOF) {
			fprintf(stderr, "Error on reading client stream: %s.\n", 
				uv_strerror(nread));
		}

		uv_close((uv_handle_t *) stream, NULL);
	}   
}
