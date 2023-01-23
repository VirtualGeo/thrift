from conans import ConanFile, CMake, MSBuild, tools
import shutil, os, stat

class ThriftConan(ConanFile):
	name = "thrift"
	version = "0.12.0"
	description = ""
	url = "https://github.com/VirtualGeo/thrift"
	homepage = "https://thrift.apache.org"
	license = "Apache License 2.0"
	exports_sources = ["build/*", "compiler/*", "lib/*", "test/*", "CMakeLists.txt", "configure.ac", "LICENSE", "README.md"]
	generators = "cmake"
	settings = "os", "arch", "compiler", "build_type"

	def requirements(self):
		self.requires("boost/1.66.0@dxt/stable")
		self.requires("OpenSSL/1.1.1a@dxt/stable")

		if self.settings.os == "Windows":
			self.requires("winflexbison/2.5.16@dxt/stable")

		#self.options["OpenSSL"].shared = False
		self.options["boost"].shared = False

	def source(self):
		shutil.move("CMakeLists.txt", "CMakeListsOriginal.txt")
		shutil.move(os.path.join("build", "conan", "CMakeLists.txt"), "CMakeLists.txt")

		# Set gradlew executable to be executable
		if self.settings.os != "Windows":
			gradlew = os.path.join("lib", "java", "gradlew")
			st = os.stat(gradlew)
			os.chmod(gradlew, st.st_mode | stat.S_IEXEC)

	def configure_cmake(self):
		cmake = CMake(self)

		cmake.definitions["WITH_MT"] = False
		cmake.definitions["WITH_STATIC_LIB"] = True
		cmake.definitions["WITH_SHARED_LIB"] = False
		cmake.definitions["WITH_BOOST_STATIC"] = True
		cmake.definitions["WITH_ZLIB"] = False
		cmake.definitions["WITH_LIBEVENT"] = False
		cmake.definitions["WITH_QT4"] = False
		cmake.definitions["WITH_QT5"] = False
		cmake.definitions["BUILD_TESTING"] = False
		cmake.definitions["BUILD_EXAMPLES"] = False
		cmake.definitions["BUILD_TUTORIALS"] = False
		cmake.definitions["WITH_JAVA"] = True
		cmake.definitions["BUILD_PYTHON"] = False
		cmake.definitions["THRIFT_COMPILER_C_GLIB"] = False

		cmake.configure()
		return cmake

	def build(self):
		cmake = self.configure_cmake()
		cmake.build()

		if self.settings.os == "Windows":
			msbuild = MSBuild(self)
			path = os.path.join(self.source_folder, "lib", "csharp", "src", "Thrift.sln")
			msbuild.build(path, properties={"OutputPath": os.path.join(self.build_folder, "csharp_dll")})

	def package(self):
		cmake = self.configure_cmake()
		cmake.install()

		self.copy("*.dll", src=os.path.join(self.build_folder, "csharp_dll"), dst="csharp")

	def package_info(self):
		self.cpp_info.libs = tools.collect_libs(self)
		self.user_info.thrift_exe = os.path.join("bin", "thrift")
		self.user_info.jar_path = os.path.join("lib", "java")
		jar_list = [
			"commons-codec-1.9",
			"commons-logging-1.2",
			"httpclient-4.4.1",
			"httpcore-4.4.1",
			"libthrift-0.12.0",
			"log4j-1.2.17",
			"servlet-api-2.5",
			"slf4j-api-1.7.12",
			"slf4j-log4j12-1.7.12",
		]
		self.user_info.jars = ",".join(jar_list)
		print(self.user_info.jars)

		if self.settings.os == "Windows":
			self.user_info.csharp_dll = os.path.join("csharp", "Thrift.dll")
