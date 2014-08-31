-module(nanomsg_nif).

-export([socket/1, close/1, setsockopt/4, getsockopt/3,
    bind/2, connect/2, shutdown/2, send/3, recv/2]).
-define(NOT_LOADED, not_loaded(?LINE)).

-on_load(init/0).

init() ->
    PrivDir = case code:priv_dir(?MODULE) of
        {error, _} ->
            EbinDir = filename:dirname(code:which(?MODULE)),
            AppPath = filename:dirname(EbinDir),
            filename:join(AppPath, "priv");
        Path ->
            Path
    end,
    erlang:load_nif(filename:join(PrivDir, "nanomsg"), 0).

not_loaded(Line) ->
    erlang:nif_error({not_loaded, [{module, ?MODULE}, {line, Line}]}).

socket(Protocol) ->
    nanomsg_socket(Protocol).

close(Socket) ->
    nanomsg_close(Socket).

setsockopt(Socket, Level, Opt, Value) ->
    nanomsg_setsockopt(Socket, Level, Opt, Value).

getsockopt(Socket, Level, Opt) ->
    nanomsg_getsockopt(Socket, Level, Opt).

bind(Socket, Addr) ->
    nanomsg_bind(Socket, Addr).

connect(Socket, Addr) ->
    nanomsg_connect(Socket, Addr).

shutdown(Socket, Eid) ->
    nanomsg_shutdown(Socket, Eid).

send(Socket, Bin, Flags) ->
    nanomsg_send(Socket, Bin, Flags).

recv(Socket, Flags) ->
    nanomsg_recv(Socket, Flags).


nanomsg_socket(_Protocol) ->
    ?NOT_LOADED.

nanomsg_close(_Socket) ->
    ?NOT_LOADED.

nanomsg_setsockopt(_Socket, _Level, _Opt, _Value) ->
    ?NOT_LOADED.

nanomsg_getsockopt(_Socket, _Level, _Opt) ->
    ?NOT_LOADED.

nanomsg_bind(_Socket, _Addr) ->
    ?NOT_LOADED.

nanomsg_connect(_Socket, _Addr) ->
    ?NOT_LOADED.

nanomsg_shutdown(_Socket, _Eid) ->
    ?NOT_LOADED.

nanomsg_send(_Socket, _Binary, _Flags) ->
    ?NOT_LOADED.

nanomsg_recv(_Socket, _Flags) ->
    ?NOT_LOADED.
