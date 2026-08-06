# syntax=docker/dockerfile:1

FROM debian:trixie-slim AS binaries

ARG TARGETARCH

COPY out/linux/build/dist/ \
     /tmp/amd64/

COPY out/linux-arm64/build/dist/ \
     /tmp/arm64/

RUN set -eux; \
    case "$TARGETARCH" in \
        amd64) \
            cp -a /tmp/amd64 /selected; \
            ;; \
        arm64) \
            cp -a /tmp/arm64 /selected; \
            ;; \
        *) \
            echo "Unsupported architecture: $TARGETARCH" >&2; \
            exit 1; \
            ;; \
    esac


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

# Install architecture-specific Argon, Isotope and stdlib.
COPY --from=binaries /selected/bin/argon \
     ${CHLORIDE_HOME}/bin/argon

COPY --from=binaries /selected/bin/isotope \
     ${CHLORIDE_HOME}/bin/isotope

COPY --from=binaries /selected/stdlib/ \
     ${CHLORIDE_HOME}/stdlib/

# Install Argon C headers.
COPY include/ \
     /usr/include/

RUN chmod +x \
        ${CHLORIDE_HOME}/bin/argon \
        ${CHLORIDE_HOME}/bin/isotope \
    && mkdir -p /usr/local/bin \
    && ln -s ${CHLORIDE_HOME}/bin/argon /usr/local/bin/argon \
    && ln -s ${CHLORIDE_HOME}/bin/isotope /usr/local/bin/isotope

WORKDIR /workspace