-module(nanomsg).

-export([socket/1, close/1, setsockopt/4, getsockopt/3, bind/2, connect/2, shutdown/2]).
-export([send/2, send/3, recv/1, recv/2]).
-export([socket_bind/2, socket_connect/2]).

-type sock_type() :: pub | sub | req | rep | push | pull |
                    surveyor | respondent | bus | pair.
-type addr() :: binary().
-type socket() :: non_neg_integer().
-type endpoint() :: non_neg_integer().

-type err() :: {error, errcode() | atom()}.
-type errcode() :: integer().

-type send_recv_opts() :: [send_recv_opt()].
-type send_recv_opt() :: dontwait.

-type socket_level() :: sol_socket | tcp | inproc | ipc |
                        req | sub | surveyor.
-type socket_opt() :: linger | sndbuf | rcvbuf | sndtimeo | rcvtimeo |
                    reconnect_ivl | reconnect_ivl_max | sndprio | rcvprio | ipv4only |
                    req_resend_ivl |
                    surveyor_deadline |
                    sub_subscribe | sub_unsubscribe.

-type socket_opt_value() :: integer() | binary().

-spec socket(sock_type()) -> {ok, socket()} | err().
socket(Type) ->
    nanomsg_nif:socket(Type).

-spec close(socket()) -> ok | err().
close(Socket) ->
    nanomsg_nif:close(Socket).

-spec setsockopt(socket(), socket_level(), socket_opt(), socket_opt_value()) -> ok | err().
setsockopt(Socket, Level, Opt, Value) ->
    nanomsg_nif:setsockopt(Socket, Level, Opt, Value).

-spec getsockopt(socket(), socket_level(), socket_opt()) -> {ok, socket_opt_value()} | err().
getsockopt(Socket, Level, Opt) ->
    nanomsg_nif:getsockopt(Socket, Level, Opt).

-spec bind(socket(), addr()) -> {ok, endpoint()} | err().
bind(Socket, Addr) ->
    nanomsg_nif:bind(Socket, Addr).

-spec connect(socket(), addr()) -> {ok, endpoint()} | err().
connect(Socket, Addr) ->
    nanomsg_nif:connect(Socket, Addr).

-spec shutdown(socket(), endpoint()) -> ok | err().
shutdown(Socket, Endpoint) ->
    nanomsg_nif:shutdown(Socket, Endpoint).

-spec send(socket(), iolist()) -> {ok, non_neg_integer()} | err().
send(Socket, Iolist) ->
    nanomsg_nif:send(Socket, Iolist, []).

-spec send(socket(), iolist(), send_recv_opts()) -> {ok, non_neg_integer()} | err().
send(Socket, Iolist, Opts) ->
    nanomsg_nif:send(Socket, Iolist, Opts).

-spec recv(socket()) -> {ok, binary()} | err().
recv(Socket) ->
    nanomsg_nif:recv(Socket, []).

-spec recv(socket(), send_recv_opts()) -> {ok, binary()} | err().
recv(Socket, Opts) ->
    nanomsg_nif:recv(Socket, Opts).

socket_bind_or_connect(Type, Addr, BindOrConnect) ->
    case socket(Type) of
        {ok, Socket} ->
            case BindOrConnect(Socket, Addr) of
                {ok, _Endpoint} ->
                    {ok, Socket};
                {error, Reason} ->
                    close(Socket),
                    {error, Reason}
            end;
        {error, Reason} ->
            {error, Reason}
    end.

-spec socket_bind(sock_type(), addr()) -> {ok, socket()} | err().
socket_bind(Type, Addr) ->
    socket_bind_or_connect(Type, Addr, fun bind/2).

-spec socket_connect(sock_type(), addr()) -> {ok, socket()} | err().
socket_connect(Type, Addr) ->
    socket_bind_or_connect(Type, Addr, fun connect/2).
