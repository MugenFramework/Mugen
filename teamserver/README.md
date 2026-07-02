# Mugen Teamserver

Mugen teamserver source code. Written in Go.

## Build

From the Mugen root directory:

```bash
make ts-build
```

The compiled binary will be at `./mugen`.

## Run

```bash
sudo ./mugen server --profile ./profiles/mugen.yaotl -v
sudo ./mugen server --profile ./profiles/mugen.yaotl -v --debug
```

## Docker

Build:
```bash
sudo docker build -t mugen-teamserver -f Teamserver-Dockerfile .
```

Create a data volume (optional):
```bash
sudo docker volume create mugen-c2-data
```

Run:
```bash
sudo docker run -it -d -v mugen-c2-data:/data mugen-teamserver
```

## Jenkins CI

A pre-configured Groovy pipeline is available at `assets/Mugen-Teamserver.groovy`.

Build the Jenkins Docker image:
```bash
sudo docker build -t jenkins-mugen-teamserver -f JT-Dockerfile .
sudo docker run -p8080:8080 -it -d -v mugen-cicd-data:/data jenkins-mugen-teamserver
```

Visit `localhost:8080` and create a Pipeline to build the Mugen Teamserver.

## Profile

Mugen uses profiles in the `yaotl` format (HCL-based). Default profile: `profiles/mugen.yaotl`.

```bash
sudo ./mugen server --profile ./profiles/mugen.yaotl -v --debug
```

All files created during operation are stored in `./data/`.
