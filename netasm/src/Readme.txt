** Project Description **
NetAsm provides a hook to the .NET JIT compiler and enables to inject your own native code in replacement of the default CLR JIT compilation. With this library, it is possible, at runtime, to inject x86 assembler code in CLR methods with the speed of a pure CLR method call and without the cost of Interop/PInvoke calls. 

** Features **
	- Runs on *x86 32bit Microsoft .NET platform with 2.0+ CLR runtime* (x64 may be supported in the future).
	- Provides three different native *code injection techniques*: Static, DLL, and Dynamic.
	- *Static code injection*: The native code is stored in an attribute of the method.
	- *Dll code injection* : this method is similar to the DllImport mechanism but CLR methods are directly bind to the DLL function, without going through the interop layers.
		- *Dynamic code injection*: you can generate native code dynamically with a callback interface that is called by the JIT when compilation of a method is occurring. It means that you can compile a method “on the fly”. You have also access to the IL code of the method being compiled.
		- *Supports for debugging* static and dynamic code injection.
		- *Supports for different calling conventions: StdCall, FastCall, ThisCall, Cdecl* although the default and recommended calling convention is *CLRCall*.
	- NetAsm can be used inside *any .NET language*.
	- Very *small library* <100Ko.

** Future plans **
You are welcome to vote for the following features (or any new features) or even contribute to one of them if you have a good experience in such domain:
* Add a simplified assembler to code inject textual __asm instructions from CodeInjection attribute?
* Add support for 64bit platform?
* Add support for Mono platform?

** Project Home and contact **
Go to http://www.codeplex.com/netasm to have more information
Contact author: alexandre_mutel [at] yahoo.fr