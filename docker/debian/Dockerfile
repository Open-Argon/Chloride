FROM debian:trixie-slim

LABEL org.opencontainers.image.title="Argon"
LABEL org.opencontainers.image.description="Argon programming language interpreter and Isotope tooling"
LABEL org.opencontainers.image.source="https://git.wbell.dev/Open-Argon/Chloride"

ENV CHLORIDE_HOME=/usr/local/lib/chloride

# Runtime libraries required by Argon, if any.
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Install Argon and Isotope
COPY out/linux/build/dist/bin/argon \
     ${CHLORIDE_HOME}/bin/argon

COPY out/linux/build/dist/bin/isotope \
     ${CHLORIDE_HOME}/bin/isotope

# Install the Argon standard library
COPY out/linux/build/dist/stdlib/ \
     ${CHLORIDE_HOME}/stdlib/

RUN chmod +x \
        ${CHLORIDE_HOME}/bin/argon \
        ${CHLORIDE_HOME}/bin/isotope \
    && mkdir -p /usr/local/bin \
    && ln -s ${CHLORIDE_HOME}/bin/argon /usr/local/bin/argon \
    && ln -s ${CHLORIDE_HOME}/bin/isotope /usr/local/bin/isotope

WORKDIR /workspace

CMD ["argon"]