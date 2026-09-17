# --- Stage 1: Build stage ---
FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libssl-dev \
    libjsoncpp-dev \
    uuid-dev \
    zlib1g-dev \
    postgresql-server-dev-all \
    && rm -rf /var/lib/apt/lists/*

# Install Drogon from source (Render will cache this step if it doesn't change)
RUN git clone https://github.com/drogonframework/drogon.git /tmp/drogon \
    && cd /tmp/drogon \
    && git submodule update --init \
    && mkdir build && cd build \
    && cmake .. -DCMAKE_BUILD_TYPE=Release \
    && make -j1 && make install \
    && rm -rf /tmp/drogon

# Build the application
WORKDIR /app
COPY . .

# We use a clean build directory in the container
RUN rm -rf build && mkdir build && cd build \
    && cmake .. -DCMAKE_BUILD_TYPE=Release \
    && make -j1

# --- Stage 2: Runtime stage ---
FROM ubuntu:24.04 AS runner

ENV DEBIAN_FRONTEND=noninteractive

# Install only the runtime dependencies (smaller image)
RUN apt-get update && apt-get install -y \
    libssl3 \
    libjsoncpp25 \
    libpq5 \
    uuid-runtime \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy the compiled binary from the builder stage
COPY --from=builder /app/build/url_shortener /app/url_shortener

# Create a directory for persistent or temporary logs
RUN mkdir -p /app/logs

# Expose the port (Render sets PORT env variable, our app defaults to 8080)
EXPOSE 8080

# Start the server
ENTRYPOINT ["/app/url_shortener"]

