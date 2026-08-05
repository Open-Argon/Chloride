FROM debian:trixie-slim

LABEL org.opencontainers.image.title="Argon"
LABEL org.opencontainers.image.description="Argon programming language interpreter and Isotope tooling"
LABEL org.opencontainers.image.source="https://git.wbell.dev/Open-Argon/Chloride"

ENV CHLORIDE_HOME=/usr/local/lib/chloride

# Runtime libraries and tools required by Argon when compiling code.
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        ca-certificates \
        gcc \
        libc6-dev \
    && rm -rf /var/lib/apt/lists/*

# Install Argon
COPY out/linux/build/dist/bin/argon \
     ${CHLORIDE_HOME}/bin/argon

# Install Isotope
COPY out/linux/build/dist/bin/isotope \
     ${CHLORIDE_HOME}/bin/isotope

# Install Argon standard library
COPY out/linux/build/dist/stdlib/ \
     ${CHLORIDE_HOME}/stdlib/

# Install Argon C headers
COPY include/ \
     /usr/include/

# Make executables available on PATH
RUN chmod +x \
        ${CHLORIDE_HOME}/bin/argon \
        ${CHLORIDE_HOME}/bin/isotope \
    && mkdir -p /usr/local/bin \
    && ln -s ${CHLORIDE_HOME}/bin/argon /usr/local/bin/argon \
    && ln -s ${CHLORIDE_HOME}/bin/isotope /usr/local/bin/isotope

WORKDIR /workspace

CMD ["argon"]