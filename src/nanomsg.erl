-module(nanomsg).

-export([test/0]).

busy_recv(Socket) ->
    case nanomsg_nif:recv(Socket, [dontwait]) of
        {error, 11} ->
            busy_recv(Socket);
        {ok, Data} ->
            {ok, Data}
    end.

test_rep_echo(Addr) ->
    {ok, Socket} = nanomsg_nif:socket(rep),
    {ok, Eid} = nanomsg_nif:bind(Socket, Addr),
    {ok, Data} = busy_recv(Socket),
    {ok, _Written} = nanomsg_nif:send(Socket, Data, []),
    ok = nanomsg_nif:shutdown(Socket, Eid),
    ok = nanomsg_nif:close(Socket).

test_req(Addr, Data) ->
    {ok, Socket} = nanomsg_nif:socket(req),
    {ok, Eid} = nanomsg_nif:connect(Socket, Addr),
    {ok, _Written} = nanomsg_nif:send(Socket, Data, []),
    {ok, Data} = busy_recv(Socket),
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
