using System.ComponentModel;
using System.Runtime.CompilerServices;
using NetAsm;
using NetAsmDemoSafeGarden;

namespace NetAsmDemoSafeGarden
{
    /// <summary>
    /// This class demonstrates the use of a Global Code Injector. This class is placed in a separate namespace (NetAsmDemoSafeGarden) in order
    /// to protect this class to be JITed by NetAsm, resulting in a recursive loop while compiling the method "CompileMethod".
    /// The hook is then installed like this <c>JITHook.Install("NetAsmDemo\\..*", new GlobalCodeInjector());</c>. 
    /// Note that the NetAsmDemoSafeGarden namespace is not in the regexp, allowing the method of this class to run without any stackoverflow issues...
    /// </summary>
    public class GlobalCodeInjector : ICodeInjector
    {
        public void CompileMethod(MethodContext context)
        {
            // Just display that the Global hook is called
            PerfManager.Instance.WriteLine("> Global JIT Code Injector is called : Compile Method {0}.{1}", context.MethodInfo.DeclaringType.Name, context.MethodInfo.ToString());
            // Nothing to do. Let the default JIT compile the method.
        }
    }
}

namespace NetAsmDemo
{
    /// <summary>
    /// Demonstrate global code injection. The <see cref="GlobalCodeInjector"/> is used to hook the compilation for the method of this class.
    /// For the demonstration, all the methods are forced not to inline.
    /// Note that this class doesn't have any attribute like <see cref="AllowCodeInjectionAttribute"/> or <see cref="CodeInjectionAttribute"/>.
    /// The Global Code Injector use a pattern matching on class names to determine which class is going to go through the Global
    /// Code Injector.
    /// </summary>
    public class TestGlobalInjection
    {
        [MethodImpl(MethodImplOptions.NoInlining)]
        static private void RunPrivateMethod()
        {
            PerfManager.Instance.WriteLine(" RunPrivateMethod called");
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        static protected void RunProtectedMethod()
        {
            PerfManager.Instance.WriteLine(" RunProtectedMethod called");
            RunPrivateMethod();
        }

        [Description("Global Injection"), MethodImpl(MethodImplOptions.NoInlining)]
        static public void RunPublicMethod()
        {
            PerfManager.Instance.WriteLine(" RunPublicMethod called");
            RunProtectedMethod();
        }
    }
}
