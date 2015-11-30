FROM gcc:5.2.0
COPY . /usr/src/datahubapi
WORKDIR /usr/src/datahubapi
RUN yum install -y automake && yum install -y autoconf && yum install -y make && yum install -y cmake && yum install -y libtool
RUN yum clean
RUN make
CMD ["./datahubapi"]
