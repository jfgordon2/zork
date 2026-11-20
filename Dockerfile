FROM devkitpro/devkitarm:20260610

RUN apt-get update \
    && apt-get install -y --no-install-recommends python3-pil \
    && rm -rf /var/lib/apt/lists/*

ENV DEVKITARM=/opt/devkitpro/devkitARM \
    CROSS=/opt/devkitpro/devkitARM/bin/arm-none-eabi- \
    PYTHON=python3

WORKDIR /workspace

ENTRYPOINT ["make"]
CMD ["all"]
