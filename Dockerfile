FROM gcc:5.2.0
COPY . /usr/src/datahubapi
WORKDIR /usr/src/datahubapi
RUN gcc -o datahubapi main.c
CMD ["./datahubapi"]
