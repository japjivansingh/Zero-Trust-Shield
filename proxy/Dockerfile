FROM gcc:latest
WORKDIR /app
COPY proxy.c .
RUN gcc -o proxy proxy.c
EXPOSE 33060
CMD ["./proxy"]

