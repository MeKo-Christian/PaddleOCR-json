# PaddleOCR-json V1.4 Docker Deployment Guide

You can deploy this project to a Docker container, then connect and use it through API or socket.

Deployment steps:

1. [Install Docker](https://yeasy.gitbook.io/docker_practice/install)
2. Clone this repository, **then open a terminal (or PowerShell) in the `cpp` folder**.
3. Next, we use Docker to build the image

```sh
docker build -t paddleocr-json .
```

> [!NOTE]
> The image build will by default enable the remote shutdown server feature and disable the function to read images from paths. The clipboard reading function is disabled by default and doesn't exist on Linux.

> [!TIP]
> You can use docker `--build-arg` parameters to enable/disable some features of the image.
>
> * `ENABLE_REMOTE_EXIT`: Controls whether to enable the remote shutdown engine process command, [see details here](README.md#cmake-build-parameters). Enabled by default.
> * `ENABLE_JSON_IMAGE_PATH`: Controls whether to enable the json command image_path, [see details here](README.md#cmake-build-parameters). Disabled by default.
>
> For example, the modified command below. It will disable remote shutdown server and image_path json command, turning this image into a pure server image. Users cannot easily shutdown the server or make the server read files inside the container.
>
> ```sh
> docker build -t paddleocr-json \
>     --build-arg "ENABLE_REMOTE_EXIT=OFF" \
>     --build-arg "ENABLE_JSON_IMAGE_PATH=OFF" \
>     .
> ```
>
> * The last `.` must be placed at the end of the entire command, it specifies the current folder as the base folder for docker image building.
> [Learn more about `--build-arg`](https://yeasy.gitbook.io/docker_practice/image/dockerfile/arg)

> [!NOTE]
> Possible errors:
>
> * If errors like the following appear during image building. This is most likely a network issue, it is recommended to change the docker mirror source or change DNS.
>
> ```text
> Ign:516 http://deb.debian.org/debian stable/main amd64 va-driver-all amd64 2.17.0-1
> Ign:517 http://deb.debian.org/debian stable/main amd64 vdpau-driver-all amd64 1.5-2
> Ign:518 http://deb.debian.org/debian stable/main amd64 xauth amd64 1:1.1.2-1
> Ign:519 http://deb.debian.org/debian stable/main amd64 xdg-user-dirs amd64 0.18-1
> Ign:520 http://deb.debian.org/debian stable/main amd64 zip amd64 3.0-13
> Err:402 http://deb.debian.org/debian stable/main amd64 libmunge2 amd64 0.5.15-2
> Could not connect to deb.debian.org:80 (146.75.94.132), connection timed out [IP: 146.75.94.132 80]
> Err:403 http://deb.debian.org/debian stable/main amd64 libtbbmalloc2 amd64 2021.8.0-2
> Unable to connect to deb.debian.org:80: [IP: 146.75.94.132 80]
> Err:404 http://deb.debian.org/debian stable/main amd64 libtbbbind-2-5 amd64 2021.8.0-2
> Unable to connect to deb.debian.org:80: [IP: 146.75.94.132 80]
> Err:405 http://deb.debian.org/debian stable/main amd64 libtbb12 amd64 2021.8.0-2
> ```

1. Next, you can deploy

```sh
docker run -d \
   --name paddleocr-json \
   -p 3746:3746 \
   paddleocr-json
```

* Here we use the parameter `-d` to run the container in background mode.
* Use the parameter `--name` to name the Docker container.
* Use the parameter `-p` to expose the container port `3746` to the local port `3746`. The container will by default open the socket server on container port `3746` when running.
* Finally, use the image `paddleocr-json` we just built to create the container.

> [!TIP]
>
> * You can add various PaddleOCR-json parameters at the end of the docker command above to modify the server. For more configuration parameters, please refer to [Simple Trial](../README.md#simple-trial) and [Common Configuration Parameters](../README.md#common-configuration-parameters-explanation)
> * Also, PaddleOCR-json has been installed in the container system, you can directly use `PaddleOCR-json` to run it in the container. Of course, you need the model library.
> * The container comes with a set of [model library](https://github.com/hiroi-sora/PaddleOCR-json/releases/tag/models%2Fv1.3), stored in the `/app/models` path. If you want to use your own model library, you can [use Docker to mount a data volume to the container](https://yeasy.gitbook.io/docker_practice/data_management/volume#qi-dong-yi-ge-gua-zai-shu-ju-juan-de-rong-qi), then use the parameter `-models_path` to specify the new model library path.

## Integrating PaddleOCR-json Docker Image into Your Dockerfile

You can integrate it into other Dockerfiles after completing the PaddleOCR-json Docker image build.

Assuming you need to run PaddleOCR-json and another process in one container, let's first write a simple bash script to start multiple processes in the background.

### run.sh

```sh
#!/bin/bash

# Start PaddleOCR-json in the background, listening on 127.0.0.1:1234, so your process can communicate with PaddleOCR-json through this port
PaddleOCR-json -models_path /app/models -addr loopback -port 1234 &

# Start your process in the background
./my_process &

# Wait for any process to exit
wait -n

# Exit the script with the exit status of the first exited process
exit $?
```

Next, we can write a new Dockerfile

#### Dockerfile

```dockerfile
# Base on the Docker image we built
FROM paddleocr-json:latest

# Run other commands
RUN ...

# Copy other files
COPY ....

# Copy the script we just wrote
COPY run.sh run.sh

# Start the PaddleOCR-json engine process
# Here we need to use ENTRYPOINT to override the ENTRYPOINT in the paddleocr-json image
# Using CMD will cause errors
ENTRYPOINT ["./run.sh"]
```

After that, just build the image and run it

```sh
docker build -t myimage .
```

```sh
docker run -d --name mycontainer myimage
```

For more examples, see:
>
> * [Official Documentation](https://docs.docker.com/config/containers/multi-service_container/)
> * [Tencent Cloud Article](https://cloud.tencent.com/developer/article/1683445)

## Calling OCR through API

We provide APIs in languages like Python and Java, see [README-Calling through API](../README.md/#calling-through-api). You can use the **socket mode** of these APIs to call the OCR service in Docker, just pass the container's IP and exposed port to the API interface.

## Other Issues

### [About Memory Leaks / Long-term High Memory Usage](./README.md#about-memory-leaks--long-term-high-memory-usage)

If you plan to use method 2 mentioned in the document, since the docker image will automatically build and compile the entire project during building, you just need to re-clone this repository and rebuild the docker image.
