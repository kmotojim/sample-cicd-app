# ===========================================================================
# Smart Mobility Dashboard - Container Build (UBI9)
#
# エアギャップ環境向け: <MIRROR_REGISTRY> を実際のミラーレジストリに
# 置き換えてください（例: registry.example.com）
# ===========================================================================

# --- Builder stage ---
# gcc-c++, cmake, make がプリインストールされたイメージを使用
FROM <MIRROR_REGISTRY>/udi-rhel9:latest AS builder

WORKDIR /app

COPY CMakeLists.txt ./
COPY third_party/ ./third_party/
COPY include/ ./include/
COPY src/ ./src/

# 静的リンクにより Runtime ステージでの libstdc++ インストールを不要にする
RUN mkdir build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF \
          -DCMAKE_EXE_LINKER_FLAGS="-static-libstdc++ -static-libgcc" .. && \
    cmake --build . --parallel $(nproc)

# --- Runtime stage ---
FROM <MIRROR_REGISTRY>/ubi9/ubi-minimal:latest

WORKDIR /app

COPY --from=builder /app/build/smart-mobility-backend .
COPY public/ ./public/

EXPOSE 8080

HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
    CMD curl -f http://localhost:8080/health || exit 1

USER 1001

CMD ["./smart-mobility-backend"]
