-module(nanomsg).

-export([test/0]).

test_rep_echo(Addr) ->
    {ok, Socket} = nanomsg_nif:socket(rep),
    {ok, Eid} = nanomsg_nif:bind(Socket, Addr),
    {ok, Data} = nanomsg_nif:recv(Socket, []),
    {ok, _Written} = nanomsg_nif:send(Socket, Data, []),
    ok = nanomsg_nif:shutdown(Socket, Eid),
    ok = nanomsg_nif:close(Socket).

test_req(Addr, Data) ->
    {ok, Socket} = nanomsg_nif:socket(req),
    {ok, Eid} = nanomsg_nif:connect(Socket, Addr),
    {ok, _Written} = nanomsg_nif:send(Socket, Data, []),
    {ok, Data} = nanomsg_nif:recv(Socket, []),
    ok = nanomsg_nif:shutdown(Socket, Eid),
    ok = nanomsg_nif:close(Socket).

test() ->
    BindAddr = <<"tcp://*:11000">>,
    Addr = <<"tcp://localhost:11000">>,
    Data = <<"hello">>,
    spawn_link(fun() ->
        test_rep_echo(BindAddr)
    end),
    timer:sleep(1000),
    test_req(Addr, Data).
