nanomsg-erlang
==

`nanomsg-erlang` is *low-level* erlang binding of nanomsg library.
The library is highly experimental. Use at our own risk.

Bug reports/PRs are welcome.

Requirement
-----

 * Erlang 17.0 (nanomsg-erlang uses NIF with dirty schedulers), Compiled with
   with dirty schedulear (`./configure --with-dirty-schedulers`).
 * nanomsg v0.4

Usage
-----

Please refer `src/nanomsg_tests.erl` for example usage. `nanomsg-erlang`
supports all transports (tcp/inproc/ipc) and protocols (pubsub/pipeline/reqrep/survey/bus/pair).
`nanomsg-erlang` is a low-level library, so use with care. Invalid operations
might be crash Erlang VM or causes leak on sockets/endpoints.

Todos
----

 * controlling process, active/pasive mode like `gen_tcp`
 * manager processed to keep track of sockets/endpoints
 * send/recv with timeouts, seperated process with `nn_poll`

Bugs
-----

 * Calling send/recv more than number of dirty schedulars freezes erlang VM
 * Crash on double bind (address already in use)
 * Sockets and endpoints are extreamly easy to be leaked
