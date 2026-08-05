FROM alpine:3.22

LABEL org.opencontainers.image.title="Argon"
LABEL org.opencontainers.image.description="Argon programming language interpreter and Isotope tooling"
LABEL org.opencontainers.image.source="https://git.wbell.dev/Open-Argon/Chloride"

ENV CHLORIDE_HOME=/usr/local/lib/chloride

# gcompat provides glibc compatibility for binaries built against glibc.
RUN apk add --no-cache \
        ca-certificates \
        gcompat

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