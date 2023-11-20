# HTTP/1.1 Web Server

## Configuration

## Inheritance

A block is said to be the child of another when he's declared in the block of the other like so:
```
parent_block {
	child_block {
	}
}
```

All inheritables attributes of parent_block will be set as default for child_block.
See the following list to know which attributes are inheritables.


## Location block attributes

| name | syntax | description | default value | inherited |
| ---- | ------ | ----------- | ------------- | --------- |
| auto_index | auto_index <boolean> | If HTTP path is a directory, list files and directories | false | yes |
| index | index <filename> | asked path is a directory, auto complete <directory>/<filename> | | no |
| body_limit | body_limit <number> | client body size shouldn't be greater than body_limit | unlimited | yes |
| cgi | cgi <extension> <interpreter_path> | if path of request end with <extension>, will execute asked script with <interpreter_path> and return content |  | no |
| root | root <filepath> | base folder of static files | path of webserv executor | yes |
| methods | methods <method1> <method2> ... | all allowed methods for the current location | GET | yes |
| error | error <error_nb> <error_filepath> | custom error content for code <error_nb> | default errors | no |

## Server block attributes

Server block also have access to all Location Block attributes.

| name | syntax | description |
| ---- | ------ | ----------- |
| server_name | server_name <host> | request the server host should use the server configuration |
| listen | listen <port1> <port2> | open server on given ports |
