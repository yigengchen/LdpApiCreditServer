FROM gcc:5.2.0
COPY . /usr/src/datahubapi
WORKDIR /usr/src/datahubapi
RUN apt-get install -y automake && apt-get install -y autoconf && apt-get install -y make && apt-get install -y cmake && apt-get install -y libtool
RUN apt-get clean
RUN make
CMD ["./datahubapi"]
