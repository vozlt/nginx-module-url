Nginx url encoding converting module
==========

[![License](http://img.shields.io/badge/license-BSD-brightgreen.svg)](https://github.com/vozlt/nginx-module-url/blob/master/LICENSE)

Nginx url encoding converting module

## Dependencies
* [nginx](http://nginx.org)

## Compatibility
* 1.7.x (last tested: 1.7.8)
* 1.6.x (last tested: 1.6.2)

Earlier versions is not tested.

## Installation

1. Clone the git repository.

  ```
  shell> git clone git://github.com/vozlt/nginx-module-url.git
  ```

2. Add the module to the build configuration by adding 
  `--add-module=/path/to/nginx-module-url`

3. Build the nginx binary.

4. Install the nginx binary.

## Synopsis

```Nginx
http {
    url_encoding_convert               on;
    url_encoding_convert_from          utf-8;
    url_encoding_convert_to            euc-kr;
}
```

## Description
This is an Nginx module that converts the encoding of characters in requested uri into user defined encoding(charset).
That is to convert uri to user defined encoding before accessing a file of the uri.
This module does work, if the requested file does not exist.
For example, nginx's configuration is as above, if the server's file name is "한글.txt"
encoded to euc-kr and client's requested uri is "/한글.txt" encoded to utf-8 and 
the requested uri is changed to euc-kr by the module before access to the file to generate content.
This module's working phase can be changed according to the directive url_encoding_convert_phase's option.

## Directives

### url_encoding_convert

| -   | - |
| --- | --- |
| **Syntax**  | url_encoding_convert [on\|off] |
| **Default** | - |
| **Context** | http, server, location |

Description: The module working's enable or disable.

### url_encoding_convert_from

| -   | - |
| --- | --- |
| **Syntax**  | url_encoding_convert_from \<*charset*\> |
| **Default** | utf-8 |
| **Context** | http, server, location |

Description: The encoding charset from client.

### url_encoding_convert_to

| -   | - |
| --- | --- |
| **Syntax**  | url_encoding_convert_to \<*charset*\> |
| **Default** | euc-kr |
| **Context** | http, server, location |

Description: The encoding charset from server.

### url_encoding_convert_phase

| -   | - |
| --- | --- |
| **Syntax**  | url_encoding_convert_phase [post_read\|preaccess] |
| **Default** | preaccess |
| **Context** | http, server, location |

Description: The module's working phase.


The following values are available when using this directive:

* post_read
  * It works before nginx-rewrite-module.
* preaccess
  * It works after nginx-rewrite-module.

### url_encoding_convert_alloc_size

| -   | - |
| --- | --- |
| **Syntax**  | url_encoding_convert_alloc_size \<*buffer_size*\> |
| **Default** | 0 |
| **Context** | http, server, location |

Description: The iconv output buffer size(bytes).

### url_encoding_convert_alloc_size_x

| -   | - |
| --- | --- |
| **Syntax**  | url_encoding_convert_alloc_size_x [*x4-x16*] |
| **Default** | x4 |
| **Context** | http, server, location |

Description: The iconv output buffer size will be multiplied.
The value will be multiplied by the length of the requested uri.

If has been set to both url_encoding_convert_alloc_size and url_encoding_convert_alloc_size_x(multiplied by uri.len), set the larger value of the two.

## Author
YoungJoo.Kim(김영주) [<vozltx@gmail.com>]
