FROM gcc:5.2.0
COPY . /usr/src/datahubapi/src
WORKDIR /usr/src/datahubapi/src
RUN apt-get install -y openssl && apt-get install -y openssl-devel
RUN apt-get clean
RUN make
CMD ["./datahubapi"]
