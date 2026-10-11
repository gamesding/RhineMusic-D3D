using System;
using System.Diagnostics;
using System.IO;
using System.IO.Compression;
using System.Reflection;
using System.Security.Cryptography;
using System.Text;
using System.Threading;
using System.Windows.Forms;

internal static class Standalone {
    private static bool Within(string file,string root) {return Path.GetFullPath(file).StartsWith(Path.GetFullPath(root).TrimEnd(Path.DirectorySeparatorChar)+Path.DirectorySeparatorChar,StringComparison.OrdinalIgnoreCase);}
    private static string Quote(string argument) {var text=new StringBuilder("\"");int slashes=0;foreach(char c in argument){if(c=='\\'){slashes++;continue;}if(c=='\"'){text.Append('\\',slashes*2+1);text.Append(c);}else{text.Append('\\',slashes);text.Append(c);}slashes=0;}text.Append('\\',slashes*2);text.Append('"');return text.ToString();}
    private static string Hash(Stream file) {using(var sha=SHA256.Create())return BitConverter.ToString(sha.ComputeHash(file)).Replace("-","").ToLowerInvariant();}
    private static bool Verify(string root) {
        string manifest=Path.Combine(root,"runtime-files.sha256");if(!File.Exists(manifest))return false;
        using(var expected=Assembly.GetExecutingAssembly().GetManifestResourceStream("Rhine.Native.files"))using(var actual=new FileStream(manifest,FileMode.Open,FileAccess.Read,FileShare.Read))if(expected==null||Hash(expected)!=Hash(actual))return false;
        foreach(string row in File.ReadLines(manifest,Encoding.UTF8)) {if(row.Length<67||row.Substring(64,2)!="  ")return false;string name=row.Substring(66).Replace('/',Path.DirectorySeparatorChar);if(Path.IsPathRooted(name)||name.IndexOf(':')>=0)return false;string target=Path.GetFullPath(Path.Combine(root,name));if(!Within(target,root)||!File.Exists(target)||(File.GetAttributes(target)&FileAttributes.ReparsePoint)!=0)return false;using(var file=new FileStream(target,FileMode.Open,FileAccess.Read,FileShare.Read))if(Hash(file)!=row.Substring(0,64))return false;}
        return File.Exists(Path.Combine(root,"RhineMusicD3D.exe"))&&File.Exists(Path.Combine(root,"backend","entry.mjs"))&&File.Exists(Path.Combine(root,"runtime","node","node.exe"));
    }
    private static void RemoveOwned(string target,string parent,string expected) {
        if(!Within(target,parent)||(File.GetAttributes(target)&FileAttributes.ReparsePoint)!=0||!File.Exists(Path.Combine(target,".rhine-native-owned"))||File.ReadAllText(Path.Combine(target,".rhine-native-owned")).Trim()!=expected)throw new InvalidDataException("运行时目录不属于当前程序，未作清理。");
        DeleteTree(target,parent);
    }
    private static void DeleteTree(string target,string parent) {
        if(!Within(target,parent))throw new InvalidDataException("运行时目录边界校验失败。");
        foreach(string file in Directory.GetFiles(target)){File.SetAttributes(file,FileAttributes.Normal);File.Delete(file);}
        foreach(string directory in Directory.GetDirectories(target)){if((File.GetAttributes(directory)&FileAttributes.ReparsePoint)!=0)Directory.Delete(directory);else DeleteTree(directory,parent);}
        Directory.Delete(target);
    }
    [STAThread] public static int Main(string[] args) {
        AppContext.SetSwitch("Switch.System.IO.UseLegacyPathHandling",false);AppContext.SetSwitch("Switch.System.IO.BlockLongPaths",false);bool interactive=Array.IndexOf(args,"--interactive-test")>=0;bool test=Array.IndexOf(args,"--self-test")>=0||interactive;
        try {
            var assembly=Assembly.GetExecutingAssembly();string expected;using(var reader=new StreamReader(assembly.GetManifestResourceStream("Rhine.Native.sha256")))expected=reader.ReadToEnd().Trim();if(expected.Length!=64)throw new InvalidDataException("Invalid embedded checksum");
            string baseDir=Environment.GetEnvironmentVariable("RHINE_RUNTIME_ROOT");if(String.IsNullOrWhiteSpace(baseDir))baseDir=Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),"RhineMusicD3D","runtimes");baseDir=Path.GetFullPath(baseDir);Directory.CreateDirectory(baseDir);
            string cache=Path.Combine(baseDir,expected.Substring(0,32));if(!Within(cache,baseDir))throw new InvalidDataException("Invalid native runtime directory");
            using(var mutex=new Mutex(false,"Local\\RhineMusicD3D.Runtime."+expected.Substring(0,24))) {bool held=false;try {try{held=mutex.WaitOne(TimeSpan.FromMinutes(2));}catch(AbandonedMutexException){held=true;}if(!held)throw new TimeoutException("另一实例正在准备运行文件，请稍后重新打开。");
                bool ready=Directory.Exists(cache)&&File.Exists(Path.Combine(cache,".ready"))&&File.ReadAllText(Path.Combine(cache,".ready")).Trim()==expected&&Verify(cache);
                if(!ready){string staging=Path.Combine(baseDir,expected.Substring(0,32)+".part-"+Guid.NewGuid().ToString("N"));if(!Within(staging,baseDir))throw new InvalidDataException("Invalid extraction directory");Directory.CreateDirectory(staging);File.WriteAllText(Path.Combine(staging,".rhine-native-owned"),expected);
                    try {using(var payload=assembly.GetManifestResourceStream("Rhine.Native.zip")){if(Hash(payload)!=expected)throw new InvalidDataException("内置运行文件 SHA-256 校验失败。");payload.Position=0;using(var zip=new ZipArchive(payload,ZipArchiveMode.Read,true)){foreach(var entry in zip.Entries){string name=entry.FullName.Replace('/',Path.DirectorySeparatorChar);if(String.IsNullOrWhiteSpace(name)||name.IndexOf(':')>=0||Path.IsPathRooted(name))throw new InvalidDataException("Unsafe archive entry");string target=Path.GetFullPath(Path.Combine(staging,name));if(!Within(target,staging))throw new InvalidDataException("Unsafe archive entry boundary");Directory.CreateDirectory(Path.GetDirectoryName(target));using(var input=entry.Open())using(var output=new FileStream(target,FileMode.CreateNew,FileAccess.Write,FileShare.None))input.CopyTo(output);}}}
                        if(!Verify(staging))throw new InvalidDataException("解包后的运行文件完整性校验失败。");File.WriteAllText(Path.Combine(staging,".ready"),expected);if(Directory.Exists(cache))RemoveOwned(cache,baseDir,expected);Directory.Move(staging,cache);
                    }finally{if(Directory.Exists(staging))RemoveOwned(staging,baseDir,expected);}
                }
            }finally{if(held)mutex.ReleaseMutex();}}
            if(Array.IndexOf(args,"--extract-only")>=0)return 0;
            var start=new ProcessStartInfo(Path.Combine(cache,"RhineMusicD3D.exe"));start.WorkingDirectory=cache;start.UseShellExecute=false;start.CreateNoWindow=true;start.WindowStyle=test&&!interactive?ProcessWindowStyle.Hidden:ProcessWindowStyle.Normal;foreach(string argument in args)start.Arguments+=(start.Arguments.Length==0?"":" ")+Quote(argument);using(var process=Process.Start(start)){process.WaitForExit();return process.ExitCode;}
        }catch(Exception error){if(test){int index=Array.IndexOf(args,"--report");if(index>=0&&index+1<args.Length)try{string file=Path.GetFullPath(args[index+1]);Directory.CreateDirectory(Path.GetDirectoryName(file));string message=error.ToString().Replace("\\","\\\\").Replace("\"","\\\"").Replace("\r","\\r").Replace("\n","\\n");File.WriteAllText(file,"{\"ok\":false,\"launcherError\":\""+message+"\"}");}catch{}}else MessageBox.Show(error.Message,"Rhine Music",MessageBoxButtons.OK,MessageBoxIcon.Error);return 1;}
    }
}
