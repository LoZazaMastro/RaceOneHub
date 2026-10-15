using System.IO;
using System.IO.Compression;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;

namespace ForzaMastro.Launcher;

internal static class PortablePayloadTests
{
    internal static void Run(Action<bool,string> check, string testRoot)
    {
        var root=Path.Combine(testRoot,"embedded-components");
        var body=Encoding.UTF8.GetBytes("Authored non-executable container fixture");
        byte[] Archive(string name="tools/marker.txt", string? fingerprint=null, bool extra=false, bool symlink=false)
        {
            using var memory=new MemoryStream();
            using(var zip=new ZipArchive(memory,ZipArchiveMode.Create,true))
            {
                var item=new PortablePayload.Item(name,body.Length,fingerprint??Convert.ToHexString(SHA256.HashData(body)));
                using(var manifest=zip.CreateEntry("manifest.json").Open())JsonSerializer.Serialize(manifest,new PortablePayload.Manifest(1,[item]));
                var entry=zip.CreateEntry(name);
                if(symlink)entry.ExternalAttributes=unchecked((int)0xa1ff0000);
                using(var output=entry.Open())output.Write(body);
                if(extra)using(var output=zip.CreateEntry("unlisted.txt").Open())output.WriteByte(1);
            }
            return memory.ToArray();
        }
        string Install(byte[] bytes,string hash)
        {
            using var stream=new MemoryStream(bytes);
            return PortablePayload.Extract(stream,root,hash);
        }
        var archive=Archive();var id=Convert.ToHexString(SHA256.HashData(archive)).ToLowerInvariant();
        var destination=Install(archive,id);
        check(File.ReadAllBytes(Path.Combine(destination,"tools/marker.txt")).SequenceEqual(body),"Embedded component bytes preserved");
        check(Install(archive,id)==destination,"Verified embedded cache reused");
        check(!Directory.EnumerateDirectories(root,".staging-*").Any(),"Successful extraction leaves no staging files");
        void Refused(byte[] bytes,string name,string hash)
        {
            bool rejected=false;try{Install(bytes,hash);}catch(InvalidDataException){rejected=true;}
            check(rejected,name);
            check(!Directory.Exists(Path.Combine(root,hash)),name+" never commits output");
            check(!Directory.EnumerateDirectories(root,".staging-*").Any(),name+" removes only owned staging");
        }
        Refused(Archive("../escape.txt"),"Traversal rejected",new string('1',64));
        Refused(Archive("C:/escape.txt"),"Absolute path rejected",new string('2',64));
        Refused(Archive("tools\\escape.txt"),"Backslash path rejected",new string('3',64));
        Refused(Archive(fingerprint:new string('0',64)),"Changed payload rejected",new string('4',64));
        Refused(Archive(extra:true),"Unlisted entry rejected",new string('5',64));
        Refused(Archive(symlink:true),"Symlink entry rejected",new string('6',64));
        Refused(Archive("tools/CON.txt"),"Windows device path rejected",new string('7',64));
        File.WriteAllText(Path.Combine(destination,"tools/marker.txt"),"Changed cache");
        bool changed=false;try{Install(archive,id);}catch(InvalidDataException){changed=true;}
        check(changed,"Tampered cache fails validation");
        check(File.ReadAllText(Path.Combine(destination,"tools/marker.txt"))=="Changed cache","Tampered cache not overwritten or deleted");
        check(!File.Exists(Path.Combine(testRoot,"escape.txt")),"Extraction never writes outside component root");
    }
}
