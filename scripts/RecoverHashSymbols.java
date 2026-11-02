// Correct return types and recover functions missed by the first analysis. @category GameRE
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.data.*;
import ghidra.program.model.symbol.*;
import com.google.gson.*;
import java.nio.file.*;
import java.util.*;

public class RecoverHashSymbols extends GhidraScript {
 void signature(Function f,DataType result,DataType... types) throws Exception {
  f.setCallingConvention("__cdecl");f.setReturnType(result,SourceType.USER_DEFINED);
  ArrayList<Parameter> params=new ArrayList<>();
  for(int i=0;i<types.length;i++)params.add(new ParameterImpl("arg"+i,types[i],currentProgram));
  f.replaceParameters(params,Function.FunctionUpdateType.DYNAMIC_STORAGE_ALL_PARAMS,true,SourceType.USER_DEFINED);
 }
 public void run() throws Exception {
  Path root=Path.of(getScriptArgs()[0]);Path folder=root.resolve("recovered/hash-symbols");Files.createDirectories(folder);
  Gson gson=new GsonBuilder().setPrettyPrinting().create();Address base=currentProgram.getImageBase();
  JsonArray manifest=JsonParser.parseString(Files.readString(root.resolve("re/replacements.json"))).getAsJsonArray();
  ArrayList<Function> functions=new ArrayList<>();
  for(JsonElement entry:manifest){JsonObject row=entry.getAsJsonObject();long rva=Long.decode(row.get("rva").getAsString());if(rva==0xcbfc0)continue;
   Address a=base.add(rva);Function f=getFunctionAt(a);
   if(f==null){disassemble(a);f=createFunction(a,"recovered_"+row.get("name").getAsString());}
   if(f==null)throw new IllegalStateException("Cannot define function at "+a);
   f.setName("recovered_"+row.get("name").getAsString(),SourceType.USER_DEFINED);
   f.setComment("Inferred semantic name; original symbol unknown. Assembly evidence and differential comparisons in reports/hash-contract.json, reports/hash-symbol-original.asm, tests/hash_symbol_differential.cpp. Source: "+row.get("source").getAsString()+". Observed ABI: "+row.get("abi").getAsString()+". Methods use ECX this; see reconstructed/symbol.hpp for layouts.");
   functions.add(f);
  }
  DataType u32=UnsignedIntegerDataType.dataType,u64=UnsignedLongLongDataType.dataType;
  DataType bytes=new PointerDataType(CharDataType.dataType,currentProgram.getDataTypeManager());
  signature(getFunctionAt(base.add(0x1cc380)),u32,u32,bytes,u32);
  signature(getFunctionAt(base.add(0x1cc430)),u32,u32,bytes,u32);
  signature(getFunctionAt(base.add(0x1eecf0)),u64,u64,bytes);
  signature(getFunctionAt(base.add(0x1eed40)),u64,u64,bytes);
  signature(getFunctionAt(base.add(0x57f20)),u64,u64);
  Address bufferAddress=base.add(0x1eec10);Function buffer=getFunctionAt(bufferAddress);
  if(buffer==null){disassemble(bufferAddress);buffer=createFunction(bufferAddress,"observed_crc64_signed_buffer");}
  signature(buffer,u64,u64,bytes,u32);
  buffer.setComment("NOT reconstructed. Signed-byte MOVSX index lacks mask and can address before the CRC64 table for high bytes. Do not replace with standard CRC64 without resolving this behavior. See reports/hash-contract.json.");functions.add(buffer);
  for(long rva:new long[]{0x5923f8,0x58fce0,0x592bf8}) {
   int size=rva==0x5923f8?2048:rva==0x58fce0?1024:256;
   clearListing(base.add(rva),base.add(rva+size-1));
   DataType element=rva==0x5923f8?u64:rva==0x58fce0?u32:ByteDataType.dataType;
   createData(base.add(rva),new ArrayDataType(element,256,element.getLength()));
   createLabel(base.add(rva),rva==0x5923f8?"recovered_crc64_table":rva==0x58fce0?"recovered_crc32_table":"recovered_ascii_fold_table",true);
  }
  DecompInterface di=new DecompInterface();di.openProgram(currentProgram);JsonArray outcomes=new JsonArray();
  for(Function f:functions){long rva=f.getEntryPoint().subtract(base);DecompileResults result=di.decompileFunction(f,90,monitor);
   JsonObject row=new JsonObject();row.addProperty("rva","0x"+Long.toHexString(rva));row.addProperty("name",f.getName());
   row.addProperty("body_bytes",f.getBody().getNumAddresses());row.addProperty("status",result.decompileCompleted()?"decompiled_after_hash_abi_repair":"failed");row.addProperty("message",result.getErrorMessage());
   if(result.decompileCompleted()){String name=String.format("recovered/hash-symbols/%08x.cpp",rva);Files.writeString(root.resolve(name),"// Inferred names and contracts, not original source. RVA 0x"+Long.toHexString(rva)+"\n"+result.getDecompiledFunction().getC());row.addProperty("file",name);}
   outcomes.add(row);
  }
  di.dispose();Files.writeString(root.resolve("reports/hash-symbol-decompilation.json"),gson.toJson(outcomes));
  println("Hash/symbol functions re-exported: "+outcomes.size());
 }
}
