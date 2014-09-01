-module(nanomsg_tests).

-include_lib("eunit/include/eunit.hrl").

-define(BIND_ADDR, <<"tcp://*:11000">>).
-define(CONNECT_ADDR, <<"tcp://localhost:11000">>).

reqrep_test() ->
    Data = <<"hello">>,
    DataLen = byte_size(Data),

    {ok, RepSocket} = nanomsg:socket_bind(rep, ?BIND_ADDR),
    {ok, ReqSocket} = nanomsg:socket_connect(req, ?CONNECT_ADDR),

    {ok, DataLen} = nanomsg:send(ReqSocket, Data),
    {ok, Data} = nanomsg:recv(RepSocket),
    {ok, DataLen} = nanomsg:send(RepSocket, Data),
    {ok, Data} = nanomsg:recv(ReqSocket),

    ok = nanomsg:close(ReqSocket),
    ok = nanomsg:close(RepSocket).

pipe_test() ->
    Data = <<"hello">>,
    DataLen = byte_size(Data),

    {ok, Socket1} = nanomsg:socket_bind(push, ?BIND_ADDR),
    {ok, Socket2} = nanomsg:socket_connect(pull, ?CONNECT_ADDR),

    {ok, DataLen} = nanomsg:send(Socket1, Data),
    {ok, Data} = nanomsg:recv(Socket2),

    ok = nanomsg:close(Socket1),
    ok = nanomsg:close(Socket2).

pair_test() ->
    Data = <<"hello">>,
    DataLen = byte_size(Data),

    {ok, Socket1} = nanomsg:socket_bind(pair, ?BIND_ADDR),
    {ok, Socket2} = nanomsg:socket_connect(pair, ?CONNECT_ADDR),

    {ok, DataLen} = nanomsg:send(Socket1, Data),
    {ok, Data} = nanomsg:recv(Socket2),
{ok, DataLen} = nanomsg:send(Socket2, Data),
    {ok, Data} = nanomsg:recv(Socket1),

    ok = nanomsg:close(Socket1),
    ok = nanomsg:close(Socket2).

subscriber(Pid) ->
    {ok, Socket} = nanomsg:socket_connect(sub, ?CONNECT_ADDR),
    ok = nanomsg:setsockopt(Socket, sub, sub_subscribe, <<"">>),
    Pid ! ready,
    {ok, Data} = nanomsg:recv(Socket),
    Pid ! Data,
    ok = nanomsg:close(Socket).

receive_n(0, _) -> ok;
receive_n(N, Val) ->
    receive
        Val -> receive_n(N-1, Val)
    end.

pubsub_test() ->
    Data = <<"hello">>,
    DataLen = byte_size(Data),
    Subscribers = 5,
    Pid = self(),

    {ok, Socket} = nanomsg:socket_bind(pub, ?BIND_ADDR),
    lists:foreach(fun(_) ->
        spawn_link(fun() -> subscriber(Pid) end)
    end, lists:seq(1, Subscribers)),

    receive_n(Subscribers, ready),
    timer:sleep(1000), % It takes some time for subscribers to connect
    {ok, DataLen} = nanomsg:send(Socket, Data),

    receive_n(Subscribers, Data),
    ok = nanomsg:close(Socket).

respondent(Pid) ->
    {ok, Socket} = nanomsg:socket_connect(respondent, ?CONNECT_ADDR),
    Pid ! ready,
    {ok, Data} = nanomsg:recv(Socket),
    DataLen = byte_size(Data),
    {ok, DataLen} = nanomsg:send(Socket, Data),
    Pid ! Data,
    ok = nanomsg:close(Socket).

survey_test() ->
    Data = <<"hello">>,
    DataLen = byte_size(Data),
    Respondents = 5,
    Pid = self(),

    {ok, Socket} = nanomsg:socket_bind(surveyor, ?BIND_ADDR),
    lists:foreach(fun(_) ->
        spawn_link(fun() -> respondent(Pid) end)
    end, lists:seq(1, Respondents)),

    receive_n(Respondents, ready),
    timer:sleep(1000), % It takes some time for respondents to connect
    {ok, DataLen} = nanomsg:send(Socket, Data),
    lists:foreach(fun(_) ->
        {ok, Data} = nanomsg:recv(Socket)
    end, lists:seq(1, Respondents)),

    receive_n(Respondents, Data),
    ok = nanomsg:close(Socket).
