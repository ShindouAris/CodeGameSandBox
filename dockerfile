FROM alpine:latest

# Toolchain
RUN apk add --no-cache gcc g++ python3

# App
WORKDIR /app
COPY build/CodeSandbox .
EXPOSE 4000/tcp

RUN chmod +x CodeSandbox
CMD ["./CodeSandbox"]