# Simple Inference Server

This program is created to serve as the easiest tool to bring any request-response application (like inference of the ML-model) into the web.
It is completely based on [drogon](https://github.com/drogonframework/drogon) http server lib.

It is useful when you have an early-stage prototype, and you want to share it with your manager who doesn't want to use docker :)

## Workflow

- Accepts `POST` request with a single `.zip` file to the root endpoint `/`
- Displays the URL to track the task if it was accepted (`/track_result?task=...`)
- When processing is finished, the track page displays the URL to download result (`get_result?task=...`)

## Usage

The actual processing should be defined as an application (or shell script), taking 2 positional arguments:
```shell
invoke.sh /path/to/input.zip /path/to/output.zip
```

The server doesn't know anything about underlying processing.
The script is given to the server as a command-line argument `--invokePath`.

A few other options are supported:
```shell
$ simple_inference_server --help
Simple Inference Server:
  -h [ --help ]           Display help message
  --invokePath arg        Path to the script that will be invoked
  --host arg (=127.0.0.1) Host to bind to
  --port arg (=7654)      Port to bind to
  --cert arg (="")        Path to SSL certificate
  --key arg (="")         Path to SSL key
```


## Application

The application is lightweight (binary size is ~3 Mb on Arch Linux) and has very few dependencies:
```shell
$ ldd simple_inference_server
        linux-vdso.so.1 (0x000072b04509d000)
        libfmt.so.10 => /usr/lib/libfmt.so.10 (0x000072b044d90000)
        libssl.so.3 => /usr/lib/libssl.so.3 (0x000072b044cb6000)
        libcrypto.so.3 => /usr/lib/libcrypto.so.3 (0x000072b044600000)
        libcares.so.2 => /usr/lib/libcares.so.2 (0x000072b044c7f000)
        libjsoncpp.so.25 => /usr/lib/libjsoncpp.so.25 (0x000072b044c43000)
        libuuid.so.1 => /usr/lib/libuuid.so.1 (0x000072b04508d000)
        libz.so.1 => /usr/lib/libz.so.1 (0x000072b044c2a000)
        libstdc++.so.6 => /usr/lib/libstdc++.so.6 (0x000072b044200000)
        libm.so.6 => /usr/lib/libm.so.6 (0x000072b044b3b000)
        libgcc_s.so.1 => /usr/lib/libgcc_s.so.1 (0x000072b044b0d000)
        libc.so.6 => /usr/lib/libc.so.6 (0x000072b04400f000)
        /lib64/ld-linux-x86-64.so.2 => /usr/lib64/ld-linux-x86-64.so.2 (0x000072b04509f000)
```

## Example of WebForm

The simplest web form that may serve as an interface to this server (provided that it is running on [localhost](http://localhost:7654)) looks like this:
```html
<!DOCTYPE html>
<html lang="">
<head>
</head>

<body>
<form method="post" enctype="multipart/form-data" action="http://localhost:7654">
    <p>
        <input type="file" name="file" accept="application/zip">
    </p>
    <p>
        <input type="submit"/>
    </p>
</form>
</body>
```
