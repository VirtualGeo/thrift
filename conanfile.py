#!/usr/bin/env python
# -*- coding: utf-8 -*-

from conans import ConanFile, CMake, tools, MSBuild
import os
import shutil
import subprocess

class ThriftConan(ConanFile):
    name = "thrift"
    version = "0.12.1"
    description = "Thrift is a lightweight, \
                    language-independent software \
                    stack with an associated code \
                    generation mechanism for RPC."
    url = "https://github.com/VirtualGeo/thrift.git"
    homepage = "https://thrift.apache.org/"
    license = "Apache License 2.0"

    exports_sources = ["build/*", "compiler/*",
                       "lib/*", "CMakeLists.txt", "configure.ac",
                       "test/*", "tutorial/*",
                       "README.md", "LICENSE"]
    generators = "cmake"

    # http://thrift.apache.org/docs/install/
    requires = (
        "boost/1.66.0@conan/stable",
    )

    settings = "os", "arch", "compiler", "build_type"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "with_zlib": [True, False],
        "with_libevent": [True, False],
        "with_qt4": [True, False],
        "with_qt5": [True, False],
        "with_openssl": [True, False],
        "with_boost_functional": [True, False],
        "with_boost_smart_ptr": [True, False],
        "with_boost_static": [True, False],
        "with_boostthreads": [True, False],
        "with_stdthreads": [True, False],
        "with_c_glib": [True, False],
        "with_cpp": [True, False],
        "with_java": [True, False],
        "with_python": [True, False],
        "with_haskell": [True, False],
        "with_plugin": [True, False],
        "build_libraries": [True, False],
        "build_compiler": [True, False],
        "build_testing": [True, False],
        "build_examples": [True, False],
        "build_tutorials": [True, False],
    }

    default_options = (
        "shared=False",
        "fPIC=True",
        "with_zlib=True",
        "with_libevent=True",
        "with_qt4=False",
        "with_qt5=False",
        "with_openssl=True",
        "with_boost_functional=False",
        "with_boost_smart_ptr=False",
        "with_boost_static=False",
        "with_boostthreads=False",
        "with_stdthreads=True",
        "with_c_glib=False",
        "with_cpp=True",
        "with_java=False",
        "with_python=False",
        "with_haskell=False",
        "with_plugin=False",
        "build_libraries=True",
        "build_compiler=True",
        # Currently fails if 'True' because of too recent boost::test version (?) in package
        "build_testing=False",
        "build_examples=False",
        "build_tutorials=False",
    )

    def config_options(self):
        if self.settings.os == 'Windows':
            del self.options.fPIC

    def configure(self):
        if self.settings.compiler != 'Visual Studio' and self.options.shared:
            self.options['boost'].add_option('fPIC', 'True')

        # See: https://github.com/apache/thrift/blob/f12cacf56145e2c8f0d4429694fedf5453648089/build/cmake/DefinePlatformSpecifc.cmake
        if self.settings.os == "Windows" and self.options.shared:
            self.output.warn(
                "Thrift does not currently support shared libs on windows. Forcing static...")
            self.options.shared = False

    def requirements(self):
        if self.settings.os == 'Windows':
            self.requires("winflexbison/2.5.16@dxt/stable")
        else:
            self.requires("flex/2.6.4@bincrafters/stable")
            self.requires("bison/3.0.4@bincrafters/stable")

        if self.options.with_openssl:
            self.requires("OpenSSL/1.1.0g@conan/stable")
            self.options["OpenSSL"].shared = False
        if self.options.with_zlib:
            self.requires("zlib/1.2.11@conan/stable")
            self.options["zlib"].shared = False
        if self.options.with_libevent:
            self.requires("libevent/2.0.22@bincrafters/stable")
            self.options["libevent"].shared = False

    def source(self):
        """Wrap the origin CMake file to call conan_basic_setup"""
        shutil.move("CMakeLists.txt", "CMakeListsOriginal.txt")
        shutil.move(os.path.join("build", "conan",
                                 "CMakeLists.txt"), "CMakeLists.txt")

        # git.clone("https://github.com/VirtualGeo/thrift.git")
        # source_url = "https://github.com/VirtualGeo/thrift.git"
        # tools.get("{0}/archive/{1}.tar.gz".format(source_url, self.version))
        # extracted_dir = self.name + "-" + self.version

        # Rename to "source_subfolder" is a convention to simplify later steps
        # os.rename(extracted_dir, self.source_subfolder)

    def configure_cmake(self):
        def add_cmake_option(option, value):
            var_name = "{}".format(option).upper()
            value_str = "{}".format(value)
            var_value = "ON" if value_str == 'True' else "OFF" if value_str == 'False' else value_str
            cmake.definitions[var_name] = var_value

        cmake = CMake(self)

        if self.settings.os != 'Windows':
            cmake.definitions['CMAKE_POSITION_INDEPENDENT_CODE'] = self.options.fPIC

        for option, value in self.options.items():
            add_cmake_option(option, value)

        # Make thrift use correct thread lib (see repo/build/cmake/config.h.in)
        add_cmake_option("USE_STD_THREAD", self.options.with_stdthreads)
        add_cmake_option("USE_BOOST_THREAD", self.options.with_boostthreads)

        add_cmake_option("WITH_SHARED_LIB", self.options.shared)
        add_cmake_option("WITH_STATIC_LIB", not self.options.shared)
        cmake.definitions["BOOST_ROOT"] = self.deps_cpp_info['boost'].rootpath

        # Make optional libs "findable"
        if self.options.with_openssl:
            cmake.definitions["OPENSSL_ROOT_DIR"] = self.deps_cpp_info['OpenSSL'].rootpath
        if self.options.with_zlib:
            cmake.definitions["ZLIB_ROOT"] = self.deps_cpp_info['zlib'].rootpath
        if self.options.with_libevent:
            cmake.definitions["LIBEVENT_ROOT"] = self.deps_cpp_info['libevent'].rootpath

        # cmake.configure(build_folder=self.build_subfolder)
        cmake.configure()
        return cmake

    def build(self):
        # cmake = self.configure_cmake()
        # cmake.build()

        if self.options.build_testing:
            self.output.info("Running {} tests".format(self.name))
            # source_path = os.path.join(
            #     self.build_subfolder, self.source_subfolder)
            # with tools.chdir(source_path):
            #     self.run(
            #         "ctest --build-config {}".format(self.settings.build_type))

        msbuild = MSBuild(self)
        path = os.path.join("lib", "csharp", "src", "Thrift.sln")
        if not self.in_local_cache:
            path = os.path.join("..", path)
        # subprocess.call("dotnet msbuild %s /nologo /v:q /t:Thrift /p:platform=%s"
            # % (path, "x86" if self.settings.arch == "x86" else "x64"), shell=False, env=None)
        msbuild.build(path, force_vcvars=True)

    def package(self):
        cmake = self.configure_cmake()
        cmake.install()
        self.copy(pattern="LICENSE", dst="licenses")
        # Copy generated headers from build tree
        # Do this only if we are not using an editable package
        if self.in_local_cache:
            self.copy(pattern="*.h", dst="include", keep_path=True)
            csharp_lib_dir = os.path.join("lib", "csharp", "src", "bin",
                                          "%s" % "x86" if self.settings.arch == "x86" else "x64",
                                          "%s" %self.settings.build_type._value)
            self.copy(os.path.join(csharp_lib_dir, "Thrift.dll"), "bin")

    def package_info(self):
        compiler_path_debug = ""
        compiler_path_release = ""

        # Editable package
        if not self.in_local_cache:
            prefix = ""
            if self.settings.os == "Windows":
                if self.settings.compiler == "Visual Studio":
                    prefix += "Generated_VC"
                    if self.settings.compiler.version == 12:
                        prefix += "12"
                    elif self.settings.compiler.version == 14:
                        prefix += "14"
                    elif self.settings.compiler.version == 15:
                        prefix += "15"

                    if self.settings.arch == "x86_64":
                        prefix += "_x64"

            elif self.settings.os == "Linux":
                prefix += "Generated_Linux"

            prefix = os.path.join(self.package_folder, prefix)

            self.cpp_info.includedirs = [os.path.join("lib", "cpp", "src"), prefix]
            # self.cpp_info.bindirs = [os.path.join(prefix, "compiler", "cpp", "bin", self.settings.build_type)]
            self.cpp_info.libdirs = [os.path.join(prefix, "lib", self.settings.build_type._value)]

            # Make 'thrift' compiler available to consumers
            compiler_path_debug = os.path.join(prefix, "compiler", "cpp", "bin", "Debug")
            compiler_path_release = os.path.join(prefix, "compiler", "cpp", "bin", "Release")
            compiler_path = os.path.join(prefix, "compiler", "cpp", "bin", self.settings.build_type._value)
        else:
            # Make 'thrift' compiler available to consumers
            compiler_path = os.path.join(self.package_folder, "bin")

        self.cpp_info.libs = tools.collect_libs(self)

        # Make sure libs are link in correct order. Important thing is that libthrift/thrift is last
        # (a little naive to sort, but libthrift/thrift should end up last since rest of the libs extend it with an abbrevation: 'thriftnb', 'thriftz')
        # The library that needs symbols must be first, then the library that resolves the symbols should come after.
        self.cpp_info.libs.sort(reverse=True)
        if self.options.with_openssl and self.settings.os == "Windows":
            self.cpp_info.libs.extend(["user32.lib"])

        if self.settings.os == "Windows":
            # To avoid error C2589: '(' : illegal token on right side of '::'
            self.cpp_info.defines.append("NOMINMAX")

        self.user_info.csharp_lib_debug = os.path.join(self.package_folder, "lib", "csharp", "src", "bin", "%s" % "x86" if self.settings.arch == "x86" else "x64", "Debug")
        self.user_info.csharp_lib_release = os.path.join(self.package_folder, "lib", "csharp", "src", "bin", "%s" % "x86" if self.settings.arch == "x86" else "x64", "Release")
        # ["%s" % "Debug" if self.settings.build_type == "Debug" else "Release"] = os.path.join(
        #     self.package_folder,
        #     "lib", "netcore", "Thrift", "bin",
        #     "%s" % ("Debug" if self.settings.build_type == "Debug" else "Release"),
        #     "netstandard2.0")

        if compiler_path_release != "":
            self.env_info.path.append(compiler_path_release)
            self.user_info.compiler = compiler_path_release
        if compiler_path_debug != "" and compiler_path_debug != compiler_path_release:
            self.env_info.path.append(compiler_path_debug)
        self.env_info.path.append(compiler_path)

        self.user_info.compiler_release = compiler_path_release
        self.user_info.compiler_debug = compiler_path_debug
        self.user_info.compiler = compiler_path

