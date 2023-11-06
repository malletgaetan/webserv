## Error management

| action | reaction |
| ------ | -------- |
| connection error (write fail / read fail) | log + close connection |
| File not found | HTTP 404 |
| Request parsing failed | HTTP 400 |
| Fail to read HTTP error default | HTTP 500 => if 500.html can't be reached send default message no read() required | 



## Startup order

1 - create configuration
2 - forks and create servers
3 - servers listen


## Notes and observations

- each servers has a copy of configuration
- each client has a reference to configuration
- configuration have http errors reponses
- if any network syscall fail = wipe client
- server class job's to handle events (read/write )and timing(timeout)
- client class job's to read, write, parse requests
- client should send headers and body separately, so we don't need to buffer entire response (could cause performance issues in case of large files)
- config class job's to parse configuration, and answer global config question of client (ex: is POST request for this path registered)

## Global TODO
- writes tests in python
- benchmarks
- run tests in github actions?