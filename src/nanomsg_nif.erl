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

socket(_Protocol) ->
    ?NOT_LOADED.

close(_Socket) ->
    ?NOT_LOADED.

setsockopt(_Socket, _Level, _Opt, _Value) ->
    ?NOT_LOADED.

getsockopt(_Socket, _Level, _Opt) ->
    ?NOT_LOADED.

bind(_Socket, _Addr) ->
    ?NOT_LOADED.

connect(_Socket, _Addr) ->
    ?NOT_LOADED.

shutdown(_Socket, _Eid) ->
    ?NOT_LOADED.

send(_Socket, _Binary, _Flags) ->
    ?NOT_LOADED.

recv(_Socket, _Flags) ->
    ?NOT_LOADED.
