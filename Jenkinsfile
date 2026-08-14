// SPDX-FileCopyrightText: 2025, 2026 William Bell
//
// SPDX-License-Identifier: GPL-3.0-or-later

pipeline {
    agent { label 'debian11' }
    environment {
        GITEA_URL = 'https://git.wbell.dev'
        GITEA_REPO = 'Open-Argon/Chloride'
    }

    stages {
        stage('Checkout') {
            steps {
                cleanWs()
                script {
                    if (env.GIT_TAG) {
                        echo "Checking out tag: ${env.GIT_TAG}"
                        checkout([
                            $class: 'GitSCM',
                            branches: [[name: "refs/tags/${env.GIT_TAG}"]],
                            userRemoteConfigs: [[url: scm.userRemoteConfigs[0].url]],
                            doGenerateSubmoduleConfigurations: false,
                            extensions: [
                                [$class: 'SubmoduleUpdate', recursiveSubmodules: true]
                            ]
                        ])
                    } else {
                        echo "Checking out normal branch"
                        checkout scm
                    }
                    sh 'git submodule update --init --recursive'
                }
            }
        }

        stage('Detect Tag') {
            steps {
                script {
                    def tag = sh(script: "git describe --tags", returnStdout: true).trim()
                    echo "Tag detected: ${tag}"

                    if (tag.toLowerCase().contains('unstable')) {
                        echo "Tag contains 'unstable' → marking build UNSTABLE"
                        currentBuild.result = 'UNSTABLE'
                    }

                    currentBuild.displayName = "#${env.BUILD_NUMBER} ${tag}"
                    env.TAG_NAME = tag
                }
            }
        }

        stage('Archive Source') {
            steps {
                script {
                    def version = env.TAG_NAME ?: "dev"
                    env.OUTPUT_FILE = "archives/source.tar.gz"
                    echo "Packaging Source as: ${env.OUTPUT_FILE}"
                }
                sh '''
                mkdir -p archives
                git ls-files --recurse-submodules | tar -czf $OUTPUT_FILE -T -
                '''
                archiveArtifacts artifacts: "${env.OUTPUT_FILE}", allowEmptyArchive: false, fingerprint: true
            }
        }

        stage('Setup Conan') {
            steps {
                sh '''
                    set -e

                    echo "Checking Docker..."

                    if ! command -v docker >/dev/null 2>&1; then
                        echo "ERROR: Docker CLI was not installed"
                        exit 1
                    fi

                    docker --version

                    echo "Checking Docker daemon..."

                    if ! docker info >/dev/null 2>&1; then
                        echo "ERROR: Docker daemon is not accessible"
                        echo "Make sure /var/run/docker.sock is mounted into the Jenkins agent."
                        exit 1
                    fi

                    echo "Docker is available:"
                    docker version
                    docker buildx inspect --bootstrap

                    echo "Checking ARM64 Linux compiler..."

                    if ! command -v aarch64-linux-gnu-gcc >/dev/null 2>&1; then
                        echo "ERROR: ARM64 GCC was not installed"
                        exit 1
                    fi

                    if ! command -v aarch64-linux-gnu-g++ >/dev/null 2>&1; then
                        echo "ERROR: ARM64 G++ was not installed"
                        exit 1
                    fi

                    echo "ARM64 compiler:"
                    aarch64-linux-gnu-gcc --version

                    echo "ARM64 C++ compiler:"
                    aarch64-linux-gnu-g++ --version

                    sudo dpkg --add-architecture arm64
                    sudo apt update

                    sudo apt install \
                        libzstd-dev \
                        liblzma-dev \
                        libbz2-dev \
                        zlib1g-dev \
                        libssl-dev \
                        libxml2-dev \
                        libzstd-dev:arm64 \
                        liblzma-dev:arm64 \
                        libbz2-dev:arm64 \
                        zlib1g-dev:arm64 \
                        libssl-dev:arm64 \
                        libxml2-dev:arm64

                    python3 -m venv /tmp/venv

                    . /tmp/venv/bin/activate

                    pip install --upgrade pip
                    pip install conan

                    mkdir -p archives macos-artifacts

                    rm -rf archives/* macos-artifacts/* *.zip *.tar.gz
                '''
            }
        }

        stage('Checkout Isotope') {
            steps {
                sh '''
                    set -e
                    rm -rf isotope-src
                    git clone --depth 1 https://git.wbell.dev/Open-Argon/Isotope.git isotope-src
                '''
            }
        }
        stage('Build (Parallel)') {
            parallel {

                stage('Linux Build') {
                    environment {
                        CONAN_HOME = "${WORKSPACE}/.conan-linux"
                    }
                    stages {
                        stage('Build') {
                            steps {
                                sh '''
                                    set -e
                                    . /tmp/venv/bin/activate

                                    rm -rf out/linux $CONAN_HOME
                                    mkdir -p out/linux/build/dist/bin

                                    conan profile detect

                                    conan install . \
                                        --build=missing \
                                        -of "out/linux"

                                    conan build . \
                                        -of "out/linux"

                                    cp -r stdlib out/linux/build/dist/

                                    ./build-stdlib.sh \
                                        out/linux/build/dist/stdlib \
                                        -j \
                                        ARGON_INCLUDE="$(realpath include)"

                                    echo "Building Isotope for Linux..."

                                    cd isotope-src

                                    go build \
                                        -trimpath \
                                        -ldflags="-s -w" \
                                        -o ../out/linux/build/dist/bin/isotope \
                                        ./src
                                '''
                            }
                        }
                    }
                }

                stage('Linux ARM64 Build') {
                    environment {
                        CONAN_HOME = "${WORKSPACE}/.conan-linux-arm64"
                    }
                    stages {
                        stage('Build') {
                            steps {
                                sh '''
                                    set -e
                                    . /tmp/venv/bin/activate

                                    rm -rf out/linux-arm64 $CONAN_HOME
                                    mkdir -p out/linux-arm64/build/dist/bin

                                    conan profile detect

                                    conan install . \
                                        --profile:build=default \
                                        --profile:host=aarch64-linux-gnu.txt \
                                        --build=missing \
                                        -of "out/linux-arm64"

                                    conan build . \
                                        --profile:build=default \
                                        --profile:host=aarch64-linux-gnu.txt \
                                        -of "out/linux-arm64"

                                    cp -r stdlib out/linux-arm64/build/dist/

                                    export CC=aarch64-linux-gnu-gcc
                                    export CXX=aarch64-linux-gnu-g++
                                    export AR=aarch64-linux-gnu-ar
                                    export RANLIB=aarch64-linux-gnu-ranlib
                                    export STRIP=aarch64-linux-gnu-strip

                                    ./build-stdlib.sh \
                                        out/linux-arm64/build/dist/stdlib \
                                        -j \
                                        ARGON_INCLUDE="$(realpath include)" \
                                         CC=aarch64-linux-gnu-gcc \
                                         CXX=aarch64-linux-gnu-g++ \
                                         AR=aarch64-linux-gnu-ar \
                                         RANLIB=aarch64-linux-gnu-ranlib \
                                         STRIP=aarch64-linux-gnu-strip

                                    echo "Building Isotope for Linux ARM64..."

                                    cd isotope-src

                                    GOOS=linux \
                                    GOARCH=arm64 \
                                    CGO_ENABLED=0 \
                                    go build \
                                        -trimpath \
                                        -ldflags="-s -w" \
                                        -o ../out/linux-arm64/build/dist/bin/isotope \
                                        ./src
                                '''
                            }
                        }
                    }
                }

                stage('Windows Build') {
                    environment {
                        CONAN_HOME = "${WORKSPACE}/.conan-windows"
                    }
                    stages {
                        stage('Build') {
                            steps {
                                sh '''
                                    set -e
                                    . /tmp/venv/bin/activate

                                    rm -rf out/windows $CONAN_HOME
                                    mkdir -p out/windows/build/dist/bin

                                    conan profile detect

                                    conan install . \
                                        --profile:host=mingw-x86_64.txt \
                                        --build=missing \
                                        -of "out/windows"

                                    conan build . \
                                        --profile:host=mingw-x86_64.txt \
                                        -of "out/windows"

                                    cp -r stdlib out/windows/build/dist/

                                    ./build-stdlib.sh \
                                        out/windows/build/dist/stdlib \
                                        -j \
                                        TARGET_OS=windows \
                                        ARGON_INCLUDE="$(realpath include)"

                                    echo "Building Isotope for Windows..."

                                    cd isotope-src

                                    GOOS=windows \
                                    GOARCH=amd64 \
                                    CGO_ENABLED=0 \
                                    go build \
                                        -trimpath \
                                        -ldflags="-s -w" \
                                        -o ../out/windows/build/dist/bin/isotope.exe \
                                        ./src
                                '''
                            }
                        }
                    }
                }

                stage('macOS Build (GitHub Actions)') {
                    environment {
                        GH_TOKEN = credentials('github-pat')
                        GH_REPO  = 'open-argon/chloride'
                        WORKFLOW = 'macOS Build (Jenkins-triggered)'
                        BUILD_NAME_ARG = "${env.TAG_NAME ?: 'dev'}"
                    }
                    steps {
                        sh '''
                            set -e

                            REF=$(git describe --tags --exact-match 2>/dev/null || git rev-parse HEAD)
                            echo "Triggering macOS build for ref: $REF"

                            gh workflow run "$WORKFLOW" \
                                --repo "$GH_REPO" \
                                --ref main \
                                -f ref="$REF" \
                                -f build_name="$BUILD_NAME_ARG"

                            RUN_ID=$(gh run list \
                                --repo "$GH_REPO" \
                                --workflow "$WORKFLOW" \
                                --limit 1 \
                                --json databaseId \
                                -q '.[0].databaseId')

                            echo "Waiting for GitHub Actions run $RUN_ID"

                            gh run watch "$RUN_ID" \
                                --repo "$GH_REPO"

                            echo "Downloading macOS arm64 artifact..."
                            gh run download "$RUN_ID" \
                                --repo "$GH_REPO" \
                                --name macos-build-arm64 \
                                --dir macos-artifacts

                            # echo "Downloading macOS x86_64 artifact..."
                            # gh run download "$RUN_ID" \
                            #     --repo "$GH_REPO" \
                            #     --name macos-build-x86_64 \
                            #     --dir macos-artifacts
                        '''
                    }
                }
            }
        }


        stage('Archive arm64 Linux') {
            steps {
                script {
                    def version = env.TAG_NAME ?: "dev"
                    env.OUTPUT_FILE = "archives/argon-${version}-linux-arm64.tar.gz"
                    echo "Packaging Linux as: ${env.OUTPUT_FILE}"
                }
                sh '''
                    cp LICENSE.txt out/linux-arm64/build/dist/
                    cp -r LICENSES out/linux-arm64/build/dist/
                    cp -r include out/linux-arm64/build/dist/
                    
                    tar -czf "$OUTPUT_FILE" -C out/linux-arm64/build/dist .
                '''
                archiveArtifacts artifacts: "${env.OUTPUT_FILE}", allowEmptyArchive: false, fingerprint: true
            }
        }
        stage('Archive Linux') {
            steps {
                script {
                    def version = env.TAG_NAME ?: "dev"
                    env.OUTPUT_FILE = "archives/argon-${version}-linux-x86_64.tar.gz"
                    echo "Packaging Linux as: ${env.OUTPUT_FILE}"
                }
                sh '''
                    cp LICENSE.txt out/linux/build/dist/
                    cp -r LICENSES out/linux/build/dist/
                    cp -r include out/linux/build/dist/
                    
                    tar -czf "$OUTPUT_FILE" -C out/linux/build/dist .
                '''
                archiveArtifacts artifacts: "${env.OUTPUT_FILE}", allowEmptyArchive: false, fingerprint: true
            }
        }
        stage('Docker Images') {
            steps {
                script {
                    def version = env.TAG_NAME ?: "dev"

                    // Docker tags cannot contain some characters that Git tags may have.
                    def dockerVersion = version
                        .replaceFirst('^v', '')
                        .replaceAll(/[^a-zA-Z0-9_.-]/, '-')
                        .toLowerCase()

                    env.DOCKER_VERSION = dockerVersion
                }

                withCredentials([usernamePassword(
                    credentialsId: 'gitea-pat',
                    usernameVariable: 'GITEA_USER',
                    passwordVariable: 'GITEA_TOKEN'
                )]) {
                    sh '''
                        set -e

                        IMAGE="git.wbell.dev/open-argon/argon:${DOCKER_VERSION}"

                        echo "Logging into Gitea container registry..."

                        echo "$GITEA_TOKEN" | docker login git.wbell.dev \
                            --username "$GITEA_USER" \
                            --password-stdin

                        echo "Checking Docker Buildx..."

                        docker buildx version

                        # Create our builder if it does not already exist.
                        if ! docker buildx inspect argon-builder >/dev/null 2>&1; then
                            echo "Creating Buildx builder..."

                            docker buildx create \
                                --name argon-builder \
                                --use
                        else
                            echo "Using existing Buildx builder..."

                            docker buildx use argon-builder
                        fi

                        echo "Bootstrapping Buildx..."

                        docker buildx inspect --bootstrap

                        echo "Building multi-architecture Debian image..."

                        docker buildx build \
                            --platform linux/amd64,linux/arm64 \
                            -f docker/debian/Dockerfile \
                            -t "$IMAGE" \
                            --push \
                            .

                        echo "Successfully pushed:"
                        echo "  $IMAGE"

                        if ! echo "$DOCKER_VERSION" | grep -qi unstable; then
                            echo "Stable release detected; publishing latest..."

                            docker buildx imagetools create \
                                -t "git.wbell.dev/open-argon/argon:latest" \
                                "$IMAGE"

                            echo "Successfully pushed:"
                            echo "  git.wbell.dev/open-argon/argon:latest"
                        else
                            echo "Unstable release; not updating latest."
                        fi

                        echo "Logging out of Gitea..."

                        docker logout git.wbell.dev
                    '''
                }
            }
        }
        stage('Debian Package Build') {
            steps {
                script {
                    def version = env.TAG_NAME ?: "0.0.0-1"
                    env.DEB_VERSION = version.replaceFirst('^v', '')
                    env.OUTPUT_FILE = "archives/argon-${env.DEB_VERSION}-x86_64.deb"
                    env.PACKAGE_ROOT = "${env.WORKSPACE}/argon-${env.DEB_VERSION}-x86_64"
                }
                withCredentials([string(credentialsId: 'gitea-pat', variable: 'GITEA_TOKEN')]) {
                sh '''
                    set -e

                    INSTALL_INTERNAL="/usr/local/lib/chloride"

                    rm -rf "$PACKAGE_ROOT"

                    # Install Argon
                    DESTDIR="$PACKAGE_ROOT" cmake --install out/linux/build --prefix "$INSTALL_INTERNAL"

                    # Install stdlib
                    mkdir -p "$PACKAGE_ROOT$INSTALL_INTERNAL/stdlib"
                    cp -R out/linux/build/dist/stdlib/* \
                        "$PACKAGE_ROOT$INSTALL_INTERNAL/stdlib/"

                    # Install Argon + Isotope binaries
                    mkdir -p "$PACKAGE_ROOT$INSTALL_INTERNAL/bin"

                    cp out/linux/build/dist/bin/argon \
                        "$PACKAGE_ROOT$INSTALL_INTERNAL/bin/argon"

                    cp out/linux/build/dist/bin/isotope \
                        "$PACKAGE_ROOT$INSTALL_INTERNAL/bin/isotope"

                    chmod +x \
                        "$PACKAGE_ROOT$INSTALL_INTERNAL/bin/argon" \
                        "$PACKAGE_ROOT$INSTALL_INTERNAL/bin/isotope"

                    # Public commands
                    mkdir -p "$PACKAGE_ROOT/usr/bin"

                    printf '#!/bin/bash\\nexec "%s/bin/argon" "$@"\\n' "$INSTALL_INTERNAL" \
                        > "$PACKAGE_ROOT/usr/bin/argon"

                    printf '#!/bin/bash\\nexec "%s/bin/isotope" "$@"\\n' "$INSTALL_INTERNAL" \
                        > "$PACKAGE_ROOT/usr/bin/isotope"

                    chmod +x \
                        "$PACKAGE_ROOT/usr/bin/argon" \
                        "$PACKAGE_ROOT/usr/bin/isotope"
                    # Headers
                    mkdir -p "$PACKAGE_ROOT/usr/include"
                    cp -R include/* "$PACKAGE_ROOT/usr/include/"

                    # Debian metadata
                    mkdir -p "$PACKAGE_ROOT/DEBIAN"

                    printf 'Package: argon\\nVersion: %s\\nArchitecture: amd64\\nMaintainer: Ugric\\nDescription: Interpreter written in C for the argon programming language\\n' \
                        "$DEB_VERSION" \
                        > "$PACKAGE_ROOT/DEBIAN/control"

                    cat > "$PACKAGE_ROOT/DEBIAN/postrm" << 'EOF'
#!/bin/bash
if [ "$1" = "remove" ] || [ "$1" = "purge" ]; then
    rm -rf /usr/local/lib/chloride
fi
EOF

                    chmod +x "$PACKAGE_ROOT/DEBIAN/postrm"

                    # Build package
                    dpkg-deb --build "$PACKAGE_ROOT" "$OUTPUT_FILE"

                    # Upload
                    curl --fail --user Jenkins:$GITEA_TOKEN \
                        --upload-file "$OUTPUT_FILE" \
                        https://git.wbell.dev/api/packages/Open-Argon/debian/pool/trixie/main/upload
                '''
                }

                archiveArtifacts artifacts: "${env.OUTPUT_FILE}",
                    allowEmptyArchive: false,
                    fingerprint: true
            }
        }
        stage('Debian Package Build ARM64') {
            steps {
                script {
                    def version = env.TAG_NAME ?: "0.0.0-1"

                    env.DEB_ARM64_VERSION = version.replaceFirst('^v', '')
                    env.OUTPUT_FILE =
                        "archives/argon-${env.DEB_ARM64_VERSION}-arm64.deb"

                    env.PACKAGE_ROOT =
                        "${env.WORKSPACE}/argon-${env.DEB_ARM64_VERSION}-arm64"
                }

                withCredentials([
                    string(
                        credentialsId: 'gitea-pat',
                        variable: 'GITEA_TOKEN'
                    )
                ]) {
                    sh '''
                        set -e

                        INSTALL_INTERNAL="/usr/local/lib/chloride"

                        rm -rf "$PACKAGE_ROOT"

                        # Install Argon
                        DESTDIR="$PACKAGE_ROOT" \
                            cmake --install out/linux-arm64/build \
                            --prefix "$INSTALL_INTERNAL"

                        # Install stdlib
                        mkdir -p "$PACKAGE_ROOT$INSTALL_INTERNAL/stdlib"

                        cp -R \
                            out/linux-arm64/build/dist/stdlib/* \
                            "$PACKAGE_ROOT$INSTALL_INTERNAL/stdlib/"

                        # Install Argon + Isotope
                        mkdir -p "$PACKAGE_ROOT$INSTALL_INTERNAL/bin"

                        cp \
                            out/linux-arm64/build/dist/bin/argon \
                            "$PACKAGE_ROOT$INSTALL_INTERNAL/bin/argon"

                        cp \
                            out/linux-arm64/build/dist/bin/isotope \
                            "$PACKAGE_ROOT$INSTALL_INTERNAL/bin/isotope"

                        chmod +x \
                            "$PACKAGE_ROOT$INSTALL_INTERNAL/bin/argon" \
                            "$PACKAGE_ROOT$INSTALL_INTERNAL/bin/isotope"

                        # Public commands
                        mkdir -p "$PACKAGE_ROOT/usr/bin"

                        printf '#!/bin/bash\\nexec "%s/bin/argon" "$@"\\n' \
                            "$INSTALL_INTERNAL" \
                            > "$PACKAGE_ROOT/usr/bin/argon"

                        printf '#!/bin/bash\\nexec "%s/bin/isotope" "$@"\\n' \
                            "$INSTALL_INTERNAL" \
                            > "$PACKAGE_ROOT/usr/bin/isotope"

                        chmod +x \
                            "$PACKAGE_ROOT/usr/bin/argon" \
                            "$PACKAGE_ROOT/usr/bin/isotope"

                        # Headers
                        mkdir -p "$PACKAGE_ROOT/usr/include"

                        cp -R \
                            include/* \
                            "$PACKAGE_ROOT/usr/include/"

                        # Debian metadata
                        mkdir -p "$PACKAGE_ROOT/DEBIAN"

                        printf 'Package: argon\\nVersion: %s\\nArchitecture: arm64\\nMaintainer: Ugric\\nDescription: Interpreter written in C for the argon programming language\\n' \
                            "$DEB_ARM64_VERSION" \
                            > "$PACKAGE_ROOT/DEBIAN/control"

                        cat > "$PACKAGE_ROOT/DEBIAN/postrm" << 'EOF'
#!/bin/bash

if [ "$1" = "remove" ] || [ "$1" = "purge" ]; then
    rm -rf /usr/local/lib/chloride
fi
EOF

                        chmod +x "$PACKAGE_ROOT/DEBIAN/postrm"

                        # Build package
                        dpkg-deb \
                            --build \
                            "$PACKAGE_ROOT" \
                            "$OUTPUT_FILE"

                        # Upload
                        curl --fail \
                            --user Jenkins:$GITEA_TOKEN \
                            --upload-file "$OUTPUT_FILE" \
                            https://git.wbell.dev/api/packages/Open-Argon/debian/pool/trixie/main/upload
                    '''
                }

                archiveArtifacts artifacts: "${env.OUTPUT_FILE}",
                    allowEmptyArchive: false,
                    fingerprint: true
            }
        }
        stage('RPM Package Build') {
            steps {
                script {
                    def version = env.TAG_NAME ?: "0.0.0-1"
                    env.RPM_VERSION = version.replaceFirst('^v', '').replaceAll('-', '.')
                    env.OUTPUT_FILE = "archives/argon-${env.RPM_VERSION}-x86_64.rpm"
                    env.RPM_BUILD_ROOT = "${env.WORKSPACE}/rpmbuild"
                }

                withCredentials([
                    string(credentialsId: 'gitea-pat', variable: 'GITEA_TOKEN'),
                    file(credentialsId: 'rpm-signing-key', variable: 'GPG_KEY_FILE')
                ]) {
                    sh '''
                        set -e

                        INSTALL_INTERNAL="/usr/local/lib/chloride"

                        rm -rf "$RPM_BUILD_ROOT"

                        mkdir -p \
                            "$RPM_BUILD_ROOT/BUILD" \
                            "$RPM_BUILD_ROOT/RPMS" \
                            "$RPM_BUILD_ROOT/SOURCES" \
                            "$RPM_BUILD_ROOT/SPECS" \
                            "$RPM_BUILD_ROOT/SRPMS"

                        # Staging dir — NOT the same as rpmbuild's own BUILDROOT,
                        # which rpmbuild wipes/recreates itself before %install runs.
                        STAGE_ROOT="$RPM_BUILD_ROOT/STAGE"
                        rm -rf "$STAGE_ROOT"

                        # Install Argon
                        DESTDIR="$STAGE_ROOT" cmake --install out/linux/build \
                            --prefix "$INSTALL_INTERNAL"

                        # Strip CMake's own internal scratch/test artifacts (e.g.
                        # compiler-detection binaries left behind in submodule build
                        # dirs) so they don't get packaged or trip up brp-strip.
                        find out/linux/build/dist/stdlib -depth -type d -name CMakeFiles -exec rm -rf {} +

                        # Install stdlib
                        mkdir -p "$STAGE_ROOT$INSTALL_INTERNAL/stdlib"
                        cp -R out/linux/build/dist/stdlib/* \
                            "$STAGE_ROOT$INSTALL_INTERNAL/stdlib/"

                        # Install Argon + Isotope
                        mkdir -p "$STAGE_ROOT$INSTALL_INTERNAL/bin"

                        cp out/linux/build/dist/bin/argon \
                            "$STAGE_ROOT$INSTALL_INTERNAL/bin/argon"

                        cp out/linux/build/dist/bin/isotope \
                            "$STAGE_ROOT$INSTALL_INTERNAL/bin/isotope"

                        chmod +x \
                            "$STAGE_ROOT$INSTALL_INTERNAL/bin/argon" \
                            "$STAGE_ROOT$INSTALL_INTERNAL/bin/isotope"

                        # Public commands
                        mkdir -p "$STAGE_ROOT/usr/bin"

                        printf '#!/bin/bash\\nexec "%s/bin/argon" "$@"\\n' "$INSTALL_INTERNAL" \
                            > "$STAGE_ROOT/usr/bin/argon"

                        printf '#!/bin/bash\\nexec "%s/bin/isotope" "$@"\\n' "$INSTALL_INTERNAL" \
                            > "$STAGE_ROOT/usr/bin/isotope"

                        chmod +x \
                            "$STAGE_ROOT/usr/bin/argon" \
                            "$STAGE_ROOT/usr/bin/isotope"

                        # Headers
                        mkdir -p "$STAGE_ROOT/usr/include"
                        cp -R include/* "$STAGE_ROOT/usr/include/"

                        CHANGELOG_DATE=$(date '+%a %b %d %Y')

                        cat > "$RPM_BUILD_ROOT/SPECS/argon.spec" << SPEC
Name:           argon
Version:        ${RPM_VERSION}
Release:        1%{?dist}
Summary:        Interpreter written in C for the Argon Programming Language
License:        GPL-3.0-or-later
URL:            https://git.wbell.dev/Open-Argon/Chloride
BuildArch:      x86_64

%description
Interpreter written in C for the Argon Programming Language

%install
cp -r ${STAGE_ROOT}/* %{buildroot}/

%files
/usr/bin/argon
/usr/bin/isotope
/usr/include/
/usr/local/lib/chloride/bin/argon
/usr/local/lib/chloride/bin/isotope
/usr/local/lib/chloride/stdlib/

%changelog
* ${CHANGELOG_DATE} Jenkins <jenkins@wbell.dev> - ${RPM_VERSION}-1
- Automated build from tag ${TAG_NAME}
SPEC

                        rpmbuild --define "_topdir $RPM_BUILD_ROOT" \
                            -bb "$RPM_BUILD_ROOT/SPECS/argon.spec"

                        BUILT_RPM=$(find "$RPM_BUILD_ROOT/RPMS" \
                            -name "argon-*.rpm" | head -1)

                        mkdir -p archives
                        cp "$BUILT_RPM" "$OUTPUT_FILE"

                        # Sign RPM
                        gpg --batch --import "$GPG_KEY_FILE"

                        echo "%_gpg_name William Bell <william@wbell.dev>" \
                            > ~/.rpmmacros
                        echo "%__gpg /usr/bin/gpg" \
                            >> ~/.rpmmacros

                        rpm --addsign "$OUTPUT_FILE"

                        # Upload
                        curl --fail --user Jenkins:$GITEA_TOKEN \
                            --upload-file "$OUTPUT_FILE" \
                            https://git.wbell.dev/api/packages/Open-Argon/rpm/upload
                    '''
                }

                archiveArtifacts artifacts: "${env.OUTPUT_FILE}",
                    allowEmptyArchive: false,
                    fingerprint: true
            }
        }
        stage('RPM Package Build ARM64') {
            steps {
                script {
                    def version = env.TAG_NAME ?: "0.0.0-1"

                    env.RPM_ARM64_VERSION =
                        version.replaceFirst('^v', '').replaceAll('-', '.')

                    env.OUTPUT_FILE =
                        "archives/argon-${env.RPM_ARM64_VERSION}-arm64.rpm"

                    env.RPM_ARM64_BUILD_ROOT =
                        "${env.WORKSPACE}/rpmbuild-arm64"
                }

                withCredentials([
                    string(
                        credentialsId: 'gitea-pat',
                        variable: 'GITEA_TOKEN'
                    ),
                    file(
                        credentialsId: 'rpm-signing-key',
                        variable: 'GPG_KEY_FILE'
                    )
                ]) {
                    sh '''
                        set -e

                        INSTALL_INTERNAL="/usr/local/lib/chloride"

                        rm -rf "$RPM_ARM64_BUILD_ROOT"

                        mkdir -p \
                            "$RPM_ARM64_BUILD_ROOT/BUILD" \
                            "$RPM_ARM64_BUILD_ROOT/RPMS" \
                            "$RPM_ARM64_BUILD_ROOT/SOURCES" \
                            "$RPM_ARM64_BUILD_ROOT/SPECS" \
                            "$RPM_ARM64_BUILD_ROOT/SRPMS"

                        STAGE_ROOT="$RPM_ARM64_BUILD_ROOT/STAGE"
                        rm -rf "$STAGE_ROOT"

                        # Install Argon
                        DESTDIR="$STAGE_ROOT" \
                            cmake --install out/linux-arm64/build \
                            --prefix "$INSTALL_INTERNAL"

                        # Strip CMake's own internal scratch/test artifacts before
                        # packaging — these are wrong-architecture binaries that
                        # brp-strip can't (and shouldn't) touch.
                        find out/linux-arm64/build/dist/stdlib -depth -type d -name CMakeFiles -exec rm -rf {} +

                        # Install stdlib
                        mkdir -p "$STAGE_ROOT$INSTALL_INTERNAL/stdlib"

                        cp -R \
                            out/linux-arm64/build/dist/stdlib/* \
                            "$STAGE_ROOT$INSTALL_INTERNAL/stdlib/"

                        # Install Argon + Isotope
                        mkdir -p "$STAGE_ROOT$INSTALL_INTERNAL/bin"

                        cp \
                            out/linux-arm64/build/dist/bin/argon \
                            "$STAGE_ROOT$INSTALL_INTERNAL/bin/argon"

                        cp \
                            out/linux-arm64/build/dist/bin/isotope \
                            "$STAGE_ROOT$INSTALL_INTERNAL/bin/isotope"

                        chmod +x \
                            "$STAGE_ROOT$INSTALL_INTERNAL/bin/argon" \
                            "$STAGE_ROOT$INSTALL_INTERNAL/bin/isotope"

                        # Public commands
                        mkdir -p "$STAGE_ROOT/usr/bin"

                        printf '#!/bin/bash\\nexec "%s/bin/argon" "$@"\\n' \
                            "$INSTALL_INTERNAL" \
                            > "$STAGE_ROOT/usr/bin/argon"

                        printf '#!/bin/bash\\nexec "%s/bin/isotope" "$@"\\n' \
                            "$INSTALL_INTERNAL" \
                            > "$STAGE_ROOT/usr/bin/isotope"

                        chmod +x \
                            "$STAGE_ROOT/usr/bin/argon" \
                            "$STAGE_ROOT/usr/bin/isotope"

                        # Headers
                        mkdir -p "$STAGE_ROOT/usr/include"

                        cp -R \
                            include/* \
                            "$STAGE_ROOT/usr/include/"

                        CHANGELOG_DATE=$(date '+%a %b %d %Y')

                        cat > "$RPM_ARM64_BUILD_ROOT/SPECS/argon.spec" << SPEC
Name:           argon
Version:        ${RPM_ARM64_VERSION}
Release:        1%{?dist}
Summary:        Interpreter written in C for the Argon Programming Language
License:        GPL-3.0-or-later
URL:            https://git.wbell.dev/Open-Argon/Chloride

%description
Interpreter written in C for the Argon Programming Language

%install
cp -r ${STAGE_ROOT}/* %{buildroot}/

%files
/usr/bin/argon
/usr/bin/isotope
/usr/include/
/usr/local/lib/chloride/bin/argon
/usr/local/lib/chloride/bin/isotope
/usr/local/lib/chloride/stdlib/

%changelog
* ${CHANGELOG_DATE} Jenkins <jenkins@wbell.dev> - ${RPM_ARM64_VERSION}-1
- Automated ARM64 build from tag ${TAG_NAME}
SPEC

                        rpmbuild \
                            --target aarch64 \
                            --define "_topdir $RPM_ARM64_BUILD_ROOT" \
                            --define "__strip /usr/bin/aarch64-linux-gnu-strip" \
                            --define "__objdump /usr/bin/aarch64-linux-gnu-objdump" \
                            -bb "$RPM_ARM64_BUILD_ROOT/SPECS/argon.spec"

                        BUILT_RPM=$(find "$RPM_ARM64_BUILD_ROOT/RPMS" \
                            -name "argon-*.rpm" | head -1)

                        if [ -z "$BUILT_RPM" ]; then
                            echo "ERROR: ARM64 RPM was not produced"
                            find "$RPM_ARM64_BUILD_ROOT/RPMS" -type f -print
                            exit 1
                        fi
                        
                        mkdir -p archives

                        cp \
                            "$BUILT_RPM" \
                            "$OUTPUT_FILE"

                        # Sign RPM
                        gpg --batch --import "$GPG_KEY_FILE"

                        echo "%_gpg_name William Bell <william@wbell.dev>" \
                            > ~/.rpmmacros
                        echo "%__gpg /usr/bin/gpg" \
                            >> ~/.rpmmacros

                        rpm --addsign "$OUTPUT_FILE"

                        # Upload
                        curl --fail \
                            --user Jenkins:$GITEA_TOKEN \
                            --upload-file "$OUTPUT_FILE" \
                            https://git.wbell.dev/api/packages/Open-Argon/rpm/upload
                    '''
                }

                archiveArtifacts artifacts: "${env.OUTPUT_FILE}",
                    allowEmptyArchive: false,
                    fingerprint: true
            }
        }
        stage('Archive Windows') {
            steps {
                script {
                    def version = env.TAG_NAME ?: "dev"
                    env.OUTPUT_FILE = "archives/argon-${version}-windows-x86_64.zip"
                    echo "Packaging Windows as: ${env.OUTPUT_FILE}"
                }
                sh '''
                    set -e
                    cp LICENSE.txt out/windows/build/dist/
                    cp -r LICENSES out/windows/build/dist/
                    cp -r include out/windows/build/dist/
                    
                    (
                    cd "out/windows/build/dist" && zip -r "../../../../$OUTPUT_FILE" .
                    )
                '''
                archiveArtifacts artifacts: "${env.OUTPUT_FILE}", allowEmptyArchive: false, fingerprint: true
            }
        }

        stage('Windows Installer build') {
            steps {
                script {
                    def version = env.TAG_NAME ?: "dev"
                    env.ARGON_VERSION = "${version}"
                    env.OUTPUT_FILE = "archives/argon-${version}-windows-installer-x86_64.exe"
                    echo "Packaging Windows as: ${env.OUTPUT_FILE}"
                }
                sh '''
                    set -e
                    
                    python3 build-windows-installer.py
                    makensis -DOUTFILE="$OUTPUT_FILE" installer.nsi
                '''
                archiveArtifacts artifacts: "${env.OUTPUT_FILE}", allowEmptyArchive: false, fingerprint: true
            }
        }

        stage('Archive macOS') {
            steps {
                archiveArtifacts artifacts: 'macos-artifacts/**/*.tar.gz', fingerprint: true
            }
        }
    }

    post {
        always {
            script {
                def tag = sh(script: "git describe --tags", returnStdout: true).trim()
                echo "Detected tag: ${tag}"

                if (tag.toLowerCase().contains("unstable")) {
                    echo "Unstable tag detected"
                    currentBuild.result = "SUCCESS"
                } else {
                    echo "Stable tagged build"
                    currentBuild.description = "Stable"
                    currentBuild.result = "SUCCESS"
                }

                echo "Cleaning up argon-* directories..."

                sh '''
                    find "$WORKSPACE" \
                        -type d \
                        -name 'argon-*' \
                        -prune \
                        -exec rm -rf {} +
                '''
            }
        }
    }
}
