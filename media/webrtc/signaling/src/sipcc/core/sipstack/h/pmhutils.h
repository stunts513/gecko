/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _PMH_UTILS_H_
#define _PMH_UTILS_H_

#include "cpr_types.h"
#include "pmhdefs.h"

/*
 * Define a "stream" created from an input packet. We have to do this
 * since SIP * messages may come in over TCP or UDP, so we cannot use
 * TCP stream semantics. It is expected that a client interfaces to
 * these streams via the API functions defined below, rather than
 * directly.
 */
typedef struct
{
    char   *buff;
    char   *loc;  /* This points to the next byte that will be read */
    int32_t nbytes;
    int32_t bytes_read;
    boolean eof;
    boolean error;
} pmhRstream_t;


/*
 * Defines a write stream to write things to and finally create an
 * "output packet". It is expected that a client interfaces to
 * these streams via the API functions defined below, rather than
 * directly.
 */
typedef struct
{
    char *buff;
    int32_t nbytes;
    int32_t total_bytes;
    boolean growable;
} pmhWstream_t;

typedef struct
{
    uint16_t num_tokens;
    char **tokens;
} pmhTokens_t;

/*
 * Buf is the input buffer holding data that you want to use a "stream".
 * The stream uses buf as is, so dont free it. Free it only when done
 * ie. by calling pmhutils_rstream_delete. Returns NULL on failure.
 * buf = Input buffer containing data.
 * nbytes = Number of bytes in buf.
 */
PMH_EXTERN pmhRstream_t *pmhutils_rstream_create(char *buf, uint32_t nbytes);


/*
 * Frees internal memory. If freebuf is TRUE, it frees the
 * buffer it was created with. If not TRUE, the internal buffer
 * should be held by the user and freed when appropriate.
 */
PMH_EXTERN void pmhutils_rstream_delete(pmhRstream_t *rs, boolean freebuf);


/*
 * Returns a character from the stream, and advances internal pointer.
 * If no characters are left, returns '\0' and sets the eof to TRUE.
 */
PMH_EXTERN char pmhutils_rstream_read_byte(pmhRstream_t *);


/*
 * Returns a character buffer  from the stream,
 * and advances internal pointer.
 * If no characters are left, returns NULL  and sets the eof to TRUE.
 * This routine allocates memory which should be freed by the user.
 * nbytes = Number of bytes asked for.
 */
PMH_EXTERN char *pmhutils_rstream_read_bytes(pmhRstream_t *rs, int32_t nbytes);

/*
 * Returns a string created from a line(as terminated by \r or \n or \r\n)
 * in the stream. This will actually allocate memory for the string,
 * which should be freed by the user when done. Empty lines are returned
 * as empty strings - ie. first char is \0.
 * Updates internal pointer in the stream.
 * Returns NULL if eof has been reached on the stream
 */
PMH_EXTERN char *pmhutils_rstream_read_line(pmhRstream_t *);

/* Creates a write stream with an internal buffer of default size */
PMH_EXTERN pmhWstream_t *pmhutils_wstream_create(void);

/* Creates a write stream with a user provided buffer */
PMH_EXTERN pmhWstream_t *pmhutils_wstream_create_with_buf(char *buf,
                                                          uint32_t nbytes);

/*
 * Grows the write streams internal buffer by a default step.
 * Mostly used internally by the write functions
 */
PMH_EXTERN boolean pmhutils_wstream_grow(pmhWstream_t *);

/*
 * Frees the internal elements of the write stream,
 * including its internal buffer if freebuf is TRUE. If freebuf
 * is FALSE, the buffer should be held by the user and freed
 * when appropriate.
 */
PMH_EXTERN void pmhutils_wstream_delete(pmhWstream_t *ws, boolean freebuf);


/*
 * Writes the input string, followed by a SIP New line
 * ie. \r\n
 * Returns FALSE on failure.
 * Expects a NULL terminated string in "line".
 * Returns FALSE on failure(ie no memory etc.)
 * May result in creation of memory, which will
 * be freed when pmhutils_wstream_free_internal is called.
 */
PMH_EXTERN boolean pmhutils_wstream_write_line(pmhWstream_t *ws, char *line);

/*
 * Writes a single character to the output stream.
 * Returns FALSE on failure, TRUE on success.
 */
PMH_EXTERN boolean pmhutils_wstream_write_byte(pmhWstream_t *, char);

/*
 * Writes a buffer(of length len pointed to by buf) to
 * the stream. May result in creation of memory, which will
 * be freed when pmhutils_wstream_free_internal is called.
 * Returns FALSE on failure(ie no memory etc.)
 */
PMH_EXTERN boolean pmhutils_wstream_write_bytes(pmhWstream_t *ws, char *buf,
                                                uint32_t len);

/*
 * Returns the internal buffer, and fills in its length in nbytes.
 * This would be used just before wstream_delete(.., FALSE) in order
 * to hold the buffer.
 */
PMH_EXTERN char *pmhutils_wstream_getbuf(pmhWstream_t *, uint32_t *nbytes);

PMH_EXTERN uint32_t pmhutils_wstream_get_length(pmhWstream_t *ws);

/*
 * A tokenizer like strtok, but creates memory for every token.
 * Use only when you have to create the memory anyway.
 * Returns NULL if no tokens are found or incase memory allocation
 * fails.
 */
PMH_EXTERN pmhTokens_t *pmh_tokenize(const char *str, const char *tokens);

/*
 * Util to free all the tokens
 */
PMH_EXTERN void pmh_tokens_free(pmhTokens_t *tokens);

#endif /*_PMH_UTILS_H_*/
