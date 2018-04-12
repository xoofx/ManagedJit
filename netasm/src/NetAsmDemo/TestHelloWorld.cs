// (c) Copyright Microsoft Corporation.
// This source is subject to the Microsoft Permissive License.
// See http://www.microsoft.com/resources/sharedsource/licensingbasics/sharedsourcelicenses.mspx.
// All other rights reserved.
using System;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using NetAsm;

namespace NetAsmDemo
{
    /// <summary>
    /// Basic HelloWorld with static code injection. 
    /// </summary>
    [AllowCodeInjection]
    class TestHelloWorld
    {
        /// <summary>
        /// Use NetAsm to inject simple native code 0xC3 : RET
        /// </summary>
        [CodeInjection(new byte[] { 0xC3 }), MethodImpl(MethodImplOptions.NoInlining)]
        static public void NetAsmReturn()
        {
            throw new Exception("Do you really have an exception?");
        }

        [Description("HelloWorld")]
        static public void Run()
        {
            NetAsmReturn();
        }
    }

}
