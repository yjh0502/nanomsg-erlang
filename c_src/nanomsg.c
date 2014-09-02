/*
 * Copyright (c) 2014 Jihyun Yu <yjh0502@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <assert.h>
#include <string.h>

#include <nanomsg/nn.h>

#include <nanomsg/pubsub.h>
#include <nanomsg/reqrep.h>
#include <nanomsg/pipeline.h>
#include <nanomsg/survey.h>
#include <nanomsg/bus.h>
#include <nanomsg/pair.h>

#include <nanomsg/tcp.h>
#include <nanomsg/inproc.h>
#include <nanomsg/ipc.h>

#include <erl_nif.h>

struct nanomsg_ctx {
    ERL_NIF_TERM atom_ok;
    ERL_NIF_TERM atom_error;
    ERL_NIF_TERM atom_badarg;

    ERL_NIF_TERM atom_SOL_SOCKET;
    ERL_NIF_TERM atom_TCP;
    ERL_NIF_TERM atom_INPROC;
    ERL_NIF_TERM atom_IPC;

    // pub-sub
    ERL_NIF_TERM atom_PUB;
    ERL_NIF_TERM atom_SUB;
    ERL_NIF_TERM atom_SUB_SUBSCRIBE;
    ERL_NIF_TERM atom_SUB_UNSUBSCRIBE;

    // req-rep
    ERL_NIF_TERM atom_REQ;
    ERL_NIF_TERM atom_REP;
    ERL_NIF_TERM atom_REQ_RESEND_IVL;

    // pipeline
    ERL_NIF_TERM atom_PUSH;
    ERL_NIF_TERM atom_PULL;

    // survey
    ERL_NIF_TERM atom_SURVEYOR;
    ERL_NIF_TERM atom_RESPONDENT;
    ERL_NIF_TERM atom_SURVEYOR_DEADLINE;

    // bus
    ERL_NIF_TERM atom_BUS;

    // pair
    ERL_NIF_TERM atom_PAIR;

    // setsockopt
    ERL_NIF_TERM atom_LINGER;
    ERL_NIF_TERM atom_SNDBUF;
    ERL_NIF_TERM atom_RCVBUF;
    ERL_NIF_TERM atom_SNDTIMEO;
    ERL_NIF_TERM atom_RCVTIMEO;
    ERL_NIF_TERM atom_RECONNECT_IVL;
    ERL_NIF_TERM atom_RECONNECT_IVL_MAX;
    ERL_NIF_TERM atom_SNDPRIO;
    ERL_NIF_TERM atom_RCVPRIO;
    ERL_NIF_TERM atom_IPV4ONLY;
    ERL_NIF_TERM atom_SOCKET_NAME;

    ERL_NIF_TERM atom_DONTWAIT;
};

static ERL_NIF_TERM
make_atom(ErlNifEnv* env, const char* name)
{
    ERL_NIF_TERM ret;
    if(enif_make_existing_atom(env, name, &ret, ERL_NIF_LATIN1)) {
        return ret;
    }
    return enif_make_atom(env, name);
}

static ERL_NIF_TERM
make_ok(struct nanomsg_ctx *ctx, ErlNifEnv* env, ERL_NIF_TERM value)
{
    return enif_make_tuple2(env, ctx->atom_ok, value);
}

static ERL_NIF_TERM
make_error(struct nanomsg_ctx* ctx, ErlNifEnv* env, const char* error)
{
    return enif_make_tuple2(env, ctx->atom_error, make_atom(env, error));
}

#define ERR_CASE_RETURN(X) case X: return #X
static const char *
errno_to_string(int err) {
    // Error codes from nn.h
    switch(err) {
        ERR_CASE_RETURN(ENOTSUP);
        ERR_CASE_RETURN(EPROTONOSUPPORT);
        ERR_CASE_RETURN(ENOBUFS);
        ERR_CASE_RETURN(ENETDOWN);
        ERR_CASE_RETURN(EADDRINUSE);
        ERR_CASE_RETURN(EADDRNOTAVAIL);
        ERR_CASE_RETURN(ECONNREFUSED);
        ERR_CASE_RETURN(EINPROGRESS);
        ERR_CASE_RETURN(ENOTSOCK);
        ERR_CASE_RETURN(EAFNOSUPPORT);
        ERR_CASE_RETURN(EPROTO);
        ERR_CASE_RETURN(EAGAIN);
        ERR_CASE_RETURN(EBADF);
        ERR_CASE_RETURN(EINVAL);
        ERR_CASE_RETURN(EMFILE);
        ERR_CASE_RETURN(EFAULT);
        ERR_CASE_RETURN(EACCESS);
        ERR_CASE_RETURN(ENETRESET);
        ERR_CASE_RETURN(ENETUNREACH);
        ERR_CASE_RETURN(EHOSTUNREACH);
        ERR_CASE_RETURN(ENOTCONN);
        ERR_CASE_RETURN(EMSGSIZE);
        ERR_CASE_RETURN(ETIMEDOUT);
        ERR_CASE_RETURN(ECONNABORTED);
        ERR_CASE_RETURN(ECONNRESET);
        ERR_CASE_RETURN(ENOPROTOOPT);
        ERR_CASE_RETURN(EISCONN);
        ERR_CASE_RETURN(ESOCKTNOSUPPORT);
        ERR_CASE_RETURN(ETERM);
        ERR_CASE_RETURN(EFSM);
    }

    return "EUNKNOWN";
}

//#include <stdio.h>
static ERL_NIF_TERM
make_error_errno(struct nanomsg_ctx* ctx, ErlNifEnv* env)
{
    int err = nn_errno();
    const char *errstr = errno_to_string(err);
    ERL_NIF_TERM err_atom = make_atom(env, errstr);
    return enif_make_tuple2(env, ctx->atom_error, err_atom);
}

#define TEST_SET_BREAK(TERM, VAL, NAME) \
    if(enif_compare((TERM), ctx->atom_##NAME) == 0) { (VAL) = NN_##NAME; break; }

static ERL_NIF_TERM
nanomsg_socket(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
    if(argc != 1)
        return enif_make_badarg(env);

    int protocol, socket;
    ERL_NIF_TERM term_protocol = argv[0];
    struct nanomsg_ctx *ctx = enif_priv_data(env);

    do {
        TEST_SET_BREAK(term_protocol, protocol, PUB);
        TEST_SET_BREAK(term_protocol, protocol, SUB);
        TEST_SET_BREAK(term_protocol, protocol, REQ);
        TEST_SET_BREAK(term_protocol, protocol, REP);
        TEST_SET_BREAK(term_protocol, protocol, PUSH);
        TEST_SET_BREAK(term_protocol, protocol, PULL);
        TEST_SET_BREAK(term_protocol, protocol, SURVEYOR);
        TEST_SET_BREAK(term_protocol, protocol, RESPONDENT);
        TEST_SET_BREAK(term_protocol, protocol, BUS);
        TEST_SET_BREAK(term_protocol, protocol, PAIR);

        return enif_make_badarg(env);
    } while(0);

    socket = nn_socket(AF_SP, protocol);
    if(socket < 0)
        make_error(ctx, env, "nn_socket_failed");

    return make_ok(ctx, env, enif_make_int(env, socket));
}

static ERL_NIF_TERM
nanomsg_close(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
    if(argc != 1)
        return enif_make_badarg(env);

    int socket;
    ERL_NIF_TERM term_socket = argv[0];
    struct nanomsg_ctx *ctx = enif_priv_data(env);

    if(!enif_get_int(env, term_socket, &socket))
        return enif_make_badarg(env);

    if(nn_close(socket))
        return make_error_errno(ctx, env);

    return ctx->atom_ok;
}

static int
nanomsg_get_level(struct nanomsg_ctx *ctx, ERL_NIF_TERM term_level, int *level) {
    do {
        // socket level
        TEST_SET_BREAK(term_level, *level, SOL_SOCKET);
        // transport-spcific
        TEST_SET_BREAK(term_level, *level, TCP);
        TEST_SET_BREAK(term_level, *level, INPROC);
        TEST_SET_BREAK(term_level, *level, IPC);

        // socket-type-specific
        TEST_SET_BREAK(term_level, *level, SUB);
        TEST_SET_BREAK(term_level, *level, REQ);
        TEST_SET_BREAK(term_level, *level, SURVEYOR);
        return 0;
    } while(0);
    return 1;
}

#define OPT_TYPE_INT 0
#define OPT_TYPE_STRING 1
static int
nanomsg_get_sockopt(struct nanomsg_ctx *ctx, ERL_NIF_TERM term_opt, int *opt, int *type) {
    do {
        TEST_SET_BREAK(term_opt, *opt, LINGER);
        TEST_SET_BREAK(term_opt, *opt, SNDBUF);
        TEST_SET_BREAK(term_opt, *opt, RCVBUF);
        TEST_SET_BREAK(term_opt, *opt, SNDTIMEO);
        TEST_SET_BREAK(term_opt, *opt, RCVTIMEO);
        TEST_SET_BREAK(term_opt, *opt, RECONNECT_IVL);
        TEST_SET_BREAK(term_opt, *opt, RECONNECT_IVL_MAX);
        TEST_SET_BREAK(term_opt, *opt, SNDPRIO);
        TEST_SET_BREAK(term_opt, *opt, RCVPRIO);
        TEST_SET_BREAK(term_opt, *opt, IPV4ONLY);

        TEST_SET_BREAK(term_opt, *opt, REQ_RESEND_IVL);
        TEST_SET_BREAK(term_opt, *opt, SURVEYOR_DEADLINE);

        // string opts
        do {
            TEST_SET_BREAK(term_opt, *opt, SUB_SUBSCRIBE);
            TEST_SET_BREAK(term_opt, *opt, SUB_UNSUBSCRIBE);

            return 0;
        } while(0);

        *type = OPT_TYPE_STRING;
        return 1;
    } while(0);

    *type = OPT_TYPE_INT;
    return 1;
}

static ERL_NIF_TERM
nanomsg_setsockopt(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
    if(argc != 4)
        return enif_make_badarg(env);

    ERL_NIF_TERM term_socket = argv[0];
    ERL_NIF_TERM term_level = argv[1];
    ERL_NIF_TERM term_opt = argv[2];
    ERL_NIF_TERM term_value = argv[3];
    struct nanomsg_ctx *ctx = enif_priv_data(env);
    int socket, level, opt, type, int_value;
    ErlNifBinary bin_value;

    if(!enif_get_int(env, term_socket, &socket))
        return enif_make_badarg(env);

    if(!nanomsg_get_level(ctx, term_level, &level))
        return enif_make_badarg(env);

    if(!nanomsg_get_sockopt(ctx, term_opt, &opt, &type))
        return enif_make_badarg(env);

    if(type == OPT_TYPE_INT) {
        if(!enif_get_int(env, term_value, &int_value))
            return enif_make_badarg(env);

        if(nn_setsockopt(socket, level, opt, &int_value, sizeof(int)))
            return make_error_errno(ctx, env);

        return ctx->atom_ok;
    } else if(type == OPT_TYPE_STRING) {
        if(!enif_inspect_binary(env, term_value, &bin_value))
            return enif_make_badarg(env);

        if(nn_setsockopt(socket, level, opt, bin_value.data, bin_value.size))
            return make_error_errno(ctx, env);

        return ctx->atom_ok;
    }

    // Should not reach here
    return enif_make_badarg(env);
}

static ERL_NIF_TERM
nanomsg_getsockopt(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
    if(argc != 3)
        return enif_make_badarg(env);

    struct nanomsg_ctx *ctx = enif_priv_data(env);
    return make_error(ctx, env, "not_implemented");
}

typedef int (*bind_or_connect)(int s, const char *addr);
static ERL_NIF_TERM
nanomsg_bind_or_connect(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[], bind_or_connect func) {
    if(argc != 2)
        return enif_make_badarg(env);

    ERL_NIF_TERM term_socket = argv[0];
    ERL_NIF_TERM term_addr = argv[1];
    struct nanomsg_ctx *ctx = enif_priv_data(env);
    int socket, eid;
    ErlNifBinary bin_addr;

    if(!enif_get_int(env, term_socket, &socket))
        return enif_make_badarg(env);

    if(!enif_inspect_binary(env, term_addr, &bin_addr))
        return enif_make_badarg(env);

    char addr[bin_addr.size + 1];
    memcpy(addr, bin_addr.data, bin_addr.size);
    addr[bin_addr.size] = '\0';

    if((eid = func(socket, addr)) < 0)
        return make_error_errno(ctx, env);

    return make_ok(ctx, env, enif_make_int(env, eid));
}

static ERL_NIF_TERM
nanomsg_bind(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
    return nanomsg_bind_or_connect(env, argc, argv, &nn_bind);
}

static ERL_NIF_TERM
nanomsg_connect(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
    return nanomsg_bind_or_connect(env, argc, argv, &nn_connect);
}

static ERL_NIF_TERM
nanomsg_shutdown(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
    if(argc != 2)
        return enif_make_badarg(env);

    ERL_NIF_TERM term_socket = argv[0];
    ERL_NIF_TERM term_eid = argv[1];
    struct nanomsg_ctx *ctx = enif_priv_data(env);
    int socket, eid;

    if(!enif_get_int(env, term_socket, &socket))
        return enif_make_badarg(env);

    if(!enif_get_int(env, term_eid, &eid))
        return enif_make_badarg(env);

    if(nn_shutdown(socket, eid))
        return make_error_errno(ctx, env);

    return ctx->atom_ok;
}

static int
nanomsg_send_recv_flag(struct nanomsg_ctx *ctx, ErlNifEnv *env, ERL_NIF_TERM term_flags, int *flags) {
    ERL_NIF_TERM term_flag;
    if(!enif_is_list(env, term_flags))
        return 0;

    while(enif_get_list_cell(env, term_flags, &term_flag, &term_flags)) {
        int flag;
        do {
            TEST_SET_BREAK(term_flag, flag, DONTWAIT);
            return 0;
        } while(0);
        *flags |= flag;
    }
    return 1;
}

static ERL_NIF_TERM
nanomsg_send_recv_dirty_finalizer(ErlNifEnv *env, ERL_NIF_TERM result) {
    struct nanomsg_ctx *ctx = enif_priv_data(env);
    if(enif_compare(ctx->atom_badarg, result) == 0)
        return enif_make_badarg(env);
    return result;
}

static ERL_NIF_TERM
nanomsg_send_dirty(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
    ERL_NIF_TERM result, term_socket = argv[0], term_buf = argv[1], term_flags = argv[2];
    struct nanomsg_ctx *ctx = enif_priv_data(env);
    int socket, flags = 0, writelen;
    ErlNifBinary buf;

    // All arguments are already validated on nanomsg_send
    assert(enif_get_int(env, term_socket, &socket));
    assert(enif_inspect_binary(env, term_buf, &buf));
    assert(nanomsg_send_recv_flag(ctx, env, term_flags, &flags));

    if((writelen = nn_send(socket, buf.data, buf.size, flags)) < 0) {
        result = make_error_errno(ctx, env);
        goto done;
    }
    result = make_ok(ctx, env, enif_make_int(env, writelen));

done:
    return enif_schedule_dirty_nif_finalizer(env, result,
        nanomsg_send_recv_dirty_finalizer);
}

static ERL_NIF_TERM
nanomsg_send(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
    if(argc != 3)
        return enif_make_badarg(env);

    ERL_NIF_TERM term_socket = argv[0], term_buf = argv[1], term_flags = argv[2];
    struct nanomsg_ctx *ctx = enif_priv_data(env);
    int socket, flags = 0, writelen;
    ErlNifBinary buf;

    if(!enif_get_int(env, term_socket, &socket))
        return enif_make_badarg(env);

    if(!enif_inspect_binary(env, term_buf, &buf))
        return enif_make_badarg(env);

    if(!nanomsg_send_recv_flag(ctx, env, term_flags, &flags))
        return enif_make_badarg(env);

    // Non-blocking I/O, do not use dirty schedular
    if(flags & NN_DONTWAIT) {
        if((writelen = nn_send(socket, buf.data, buf.size, flags)) < 0)
            return make_error_errno(ctx, env);

        return make_ok(ctx, env, enif_make_int(env, writelen));
    }

    // Blocking I/O, use dirty schedular
    return enif_schedule_dirty_nif(env, ERL_NIF_DIRTY_JOB_IO_BOUND, &nanomsg_send_dirty, argc, argv);
}

static ERL_NIF_TERM
nanomsg_recv_internal(struct nanomsg_ctx *ctx, ErlNifEnv *env, int socket, int flags) {
    int readlen;
    char *buf = NULL;
    ErlNifBinary bin;

    if((readlen = nn_recv(socket, &buf, NN_MSG, flags)) < 0)
        return make_error_errno(ctx, env);

    // Erlang VM will failed it memory allocation failed,
    // There is not much a library can do...
    if(!enif_alloc_binary(readlen, &bin)) {
        nn_freemsg(buf);
        return make_error(ctx, env, "alloc_failed");
    }

    memcpy(bin.data, buf, readlen);
    nn_freemsg(buf);

    return make_ok(ctx, env, enif_make_binary(env, &bin));
}

static ERL_NIF_TERM
nanomsg_recv_dirty(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
    ERL_NIF_TERM term_socket, term_flags;
    struct nanomsg_ctx *ctx = enif_priv_data(env);
    int socket = 0, flags = 0;

    term_socket = argv[0];
    term_flags = argv[1];

    // All messages are already validated on nanomsg_recv
    assert(enif_get_int(env, term_socket, &socket));
    assert(nanomsg_send_recv_flag(ctx, env, term_flags, &flags));

    return enif_schedule_dirty_nif_finalizer(env,
        nanomsg_recv_internal(ctx, env, socket, flags),
        &nanomsg_send_recv_dirty_finalizer);
}

static ERL_NIF_TERM
nanomsg_recv(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
    int socket  = 0, flags = 0;
    ERL_NIF_TERM term_socket, term_flags;
    struct nanomsg_ctx *ctx = enif_priv_data(env);

    if(argc != 2)
        return enif_make_badarg(env);

    term_socket = argv[0];
    term_flags = argv[1];
    if(!enif_get_int(env, term_socket, &socket))
        return enif_make_badarg(env);

    if(!nanomsg_send_recv_flag(ctx, env, term_flags, &flags))
        return enif_make_badarg(env);

    // Non-blocking I/O, do not use dirty schedular
    if(flags & NN_DONTWAIT)
        return nanomsg_recv_internal(ctx, env, socket, flags);

    // Blocking I/O, use dirty schedular
    return enif_schedule_dirty_nif(env, ERL_NIF_DIRTY_JOB_IO_BOUND, &nanomsg_recv_dirty, argc, argv);
}

#define ATOM_INIT(ENV, VAL, ATOM) do { (VAL) = make_atom(ENV, ATOM); } while(0)

static int
nanomsg_load(ErlNifEnv *env, void **priv_data, ERL_NIF_TERM load_info)
{
    struct nanomsg_ctx *ctx = enif_alloc(sizeof(struct nanomsg_ctx));
    if(!ctx)
        return 1;

    ATOM_INIT(env, ctx->atom_ok, "ok");
    ATOM_INIT(env, ctx->atom_error, "error");
    ATOM_INIT(env, ctx->atom_badarg, "badarg");

    ATOM_INIT(env, ctx->atom_SOL_SOCKET, "sol_socket");
    ATOM_INIT(env, ctx->atom_TCP, "tcp");
    ATOM_INIT(env, ctx->atom_INPROC, "inproc");
    ATOM_INIT(env, ctx->atom_IPC, "ipc");

    ATOM_INIT(env, ctx->atom_PUB, "pub");
    ATOM_INIT(env, ctx->atom_SUB, "sub");
    ATOM_INIT(env, ctx->atom_SUB_SUBSCRIBE, "sub_subscribe");
    ATOM_INIT(env, ctx->atom_SUB_UNSUBSCRIBE, "sub_unsubscribe");

    ATOM_INIT(env, ctx->atom_REQ, "req");
    ATOM_INIT(env, ctx->atom_REP, "rep");
    ATOM_INIT(env, ctx->atom_REQ_RESEND_IVL, "req_resend_ivl");

    ATOM_INIT(env, ctx->atom_PUSH, "push");
    ATOM_INIT(env, ctx->atom_PULL, "pull");

    ATOM_INIT(env, ctx->atom_SURVEYOR, "surveyor");
    ATOM_INIT(env, ctx->atom_RESPONDENT, "respondent");
    ATOM_INIT(env, ctx->atom_SURVEYOR_DEADLINE, "surveyor_deadline");

    ATOM_INIT(env, ctx->atom_BUS, "bus");

    ATOM_INIT(env, ctx->atom_PAIR, "pair");

    ATOM_INIT(env, ctx->atom_LINGER, "linger");
    ATOM_INIT(env, ctx->atom_SNDBUF, "sndbuf");
    ATOM_INIT(env, ctx->atom_RCVBUF, "rcvbuf");
    ATOM_INIT(env, ctx->atom_SNDTIMEO, "sndtimeo");
    ATOM_INIT(env, ctx->atom_RCVTIMEO, "rcvtimeo");
    ATOM_INIT(env, ctx->atom_RECONNECT_IVL, "reconnect_ivl");
    ATOM_INIT(env, ctx->atom_RECONNECT_IVL_MAX, "reconnect_ivl_max");
    ATOM_INIT(env, ctx->atom_SNDPRIO, "sndprio");
    ATOM_INIT(env, ctx->atom_RCVPRIO, "rcvprio");
    ATOM_INIT(env, ctx->atom_IPV4ONLY, "ipv4only");
    ATOM_INIT(env, ctx->atom_SOCKET_NAME, "socket_name");

    ATOM_INIT(env, ctx->atom_DONTWAIT, "dontwait");

    *priv_data = ctx;
    return 0;
}

static ErlNifFunc nanomsg_exports[] = {
	{"socket", 1, nanomsg_socket},
	{"close", 1, nanomsg_close},
	{"setsockopt", 4, nanomsg_setsockopt},
	{"getsockopt", 3, nanomsg_getsockopt},
	{"bind", 2, nanomsg_bind},
	{"connect", 2, nanomsg_connect},
	{"shutdown", 2, nanomsg_shutdown},
	{"send", 3, nanomsg_send},
	{"recv", 2, nanomsg_recv},
};

ERL_NIF_INIT(nanomsg_nif, nanomsg_exports, nanomsg_load, NULL, NULL, NULL)
