# ===========================================================================
# Smart Mobility Dashboard - Container Build (UBI9)
#
# エアギャップ環境向け: ベースイメージをミラーレジストリのパスに
# 変更してください（例: <MIRROR_REGISTRY>/ubi9/ubi:latest）
# ===========================================================================

# --- Builder stage ---
FROM <MIRROR_REGISTRY>/ubi9/ubi:latest AS builder

RUN dnf install -y \
    gcc-c++ \
    make \
    cmake \
    && dnf clean all

WORKDIR /app

COPY CMakeLists.txt ./
COPY third_party/ ./third_party/
COPY include/ ./include/
COPY src/ ./src/

RUN mkdir build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF .. && \
    cmake --build . --parallel $(nproc)

# --- Runtime stage ---
FROM <MIRROR_REGISTRY>/ubi9/ubi-minimal:latest

RUN microdnf install -y libstdc++ && microdnf clean all

WORKDIR /app

COPY --from=builder /app/build/smart-mobility-backend .
COPY public/ ./public/

EXPOSE 8080

HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
    CMD curl -f http://localhost:8080/health || exit 1

USER 1001

CMD ["./smart-mobility-backend"]
