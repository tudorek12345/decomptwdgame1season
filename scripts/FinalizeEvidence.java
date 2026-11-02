// Targeted calling-convention repair and final coverage export. @category GameRE
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.data.*;
import ghidra.program.model.pcode.HighFunctionDBUtil;
import ghidra.program.model.symbol.*;
import com.google.gson.*;
import java.nio.file.*;
import java.io.*;
import java.util.*;
public class FinalizeEvidence extends GhidraScript {
 public void run() throws Exception {
  Path root=Path.of(getScriptArgs()[0]);Gson gson=new GsonBuilder().setPrettyPrinting().create();
  Address base=currentProgram.getImageBase();
  // A first-pass vtable scan mistook an unrelocated scalar for a code pointer.
  // It points at INT3 padding. Keep the rejected evidence but remove the false function.
  Function falseCandidate=getFunctionAt(base.add(0x75454));
  if(falseCandidate!=null && falseCandidate.getBody().getNumAddresses()==1 && currentProgram.getMemory().getByte(base.add(0x75454))==(byte)0xcc)
      currentProgram.getFunctionManager().removeFunction(base.add(0x75454));
  Function f=getFunctionAt(base.add(0x63e70));
  f.setCallingConvention("__stdcall");f.setStackPurgeSize(8);
  long[] sites={0x63e9c,0x63eae,0x63ebe};int[] nargs={3,2,1};
  for(int k=0;k<sites.length;k++) {
   FunctionDefinitionDataType sig=new FunctionDefinitionDataType("observed_com_call_"+k,currentProgram.getDataTypeManager());
   sig.setCallingConvention("__stdcall");sig.setReturnType(UnsignedIntegerDataType.dataType);
   ParameterDefinition[] args=new ParameterDefinition[nargs[k]];
   for(int i=0;i<args.length;i++)args[i]=new ParameterDefinitionImpl("arg"+i,UnsignedIntegerDataType.dataType,"Observed 32-bit stack argument");
   sig.setArguments(args);HighFunctionDBUtil.writeOverride(f,base.add(sites[k]),sig);
  }
  f.setComment("Indirect COM calls at RVAs 0x63e9c/0x63eae/0x63ebe have observed 3/2/1 stack arguments and callee cleanup. Applied __stdcall call-site overrides to repair stack analysis. Method semantics/types beyond argument widths remain unverified.");
  DecompInterface di=new DecompInterface();di.openProgram(currentProgram);DecompileResults r=di.decompileFunction(f,120,monitor);
  JsonObject retry=new JsonObject();retry.addProperty("rva","0x63e70");retry.addProperty("decompile_completed",r.decompileCompleted());retry.addProperty("message",r.getErrorMessage());
  if(r.decompileCompleted()) {Files.writeString(root.resolve("recovered/native-additional/repaired_00063e70.cpp"),"// RVA 0x63e70; calling-convention repair, see reports/decompiler-repair.json\n"+r.getDecompiledFunction().getC());retry.addProperty("file","recovered/native-additional/repaired_00063e70.cpp");}
  di.dispose();Files.writeString(root.resolve("reports/decompiler-repair.json"),gson.toJson(retry));
  getFunctionAt(base.add(0xcbfc0)).setName("recovered_closest_point_on_segment",SourceType.USER_DEFINED);
  getFunctionAt(base.add(0xcbfc0)).setComment("Inferred geometry semantic name. See reconstructed/geometry.cpp, reports/geometry-contract.json and tests/geometry_differential.cpp. Original symbol name unknown.");
  int length=0x57d72c-0x1000;byte[] bytes=new byte[length];currentProgram.getMemory().getBytes(base.add(0x1000),bytes);
  boolean[] decoded=new boolean[length];AddressSet region=new AddressSet(base.add(0x1000),base.add(0x57d72b));
  InstructionIterator ii=currentProgram.getListing().getInstructions(region,true);long instructionCount=0,instructionBytes=0;
  while(ii.hasNext()){Instruction i=ii.next();int offset=(int)i.getAddress().subtract(base)-0x1000;instructionCount++;for(int n=0;n<i.getLength()&&offset+n<length;n++)if(offset+n>=0&&!decoded[offset+n]){decoded[offset+n]=true;instructionBytes++;}}
  long cc=0,zero=0,nop=0,other=0;
  for(int i=0;i<length;i++)if(!decoded[i]){if(bytes[i]==(byte)0xcc)cc++;else if(bytes[i]==0)zero++;else if(bytes[i]==(byte)0x90)nop++;else other++;}
  AddressSet bodies=new AddressSet();int funcs=0;FunctionIterator fi=currentProgram.getFunctionManager().getFunctions(true);
  try(BufferedWriter index=Files.newBufferedWriter(root.resolve("reports/final-function-index.jsonl"))) {
   Gson compact=new Gson();
   while(fi.hasNext()){Function fun=fi.next();if(!region.contains(fun.getEntryPoint()))continue;funcs++;bodies.add(fun.getBody());JsonObject j=new JsonObject();j.addProperty("rva","0x"+Long.toHexString(fun.getEntryPoint().subtract(base)));j.addProperty("name",fun.getName(true));j.addProperty("body_bytes",fun.getBody().getNumAddresses());index.write(compact.toJson(j)+"\n");}
  }
  JsonObject coverage=new JsonObject();coverage.addProperty("text_virtual_bytes",length);coverage.addProperty("recognized_functions",funcs);coverage.addProperty("function_body_union_bytes",bodies.intersect(region).getNumAddresses());coverage.addProperty("decoded_instructions",instructionCount);coverage.addProperty("decoded_instruction_bytes",instructionBytes);coverage.addProperty("undecoded_cc_bytes",cc);coverage.addProperty("undecoded_zero_bytes",zero);coverage.addProperty("undecoded_nop_bytes",nop);coverage.addProperty("other_undecoded_bytes",other);coverage.addProperty("caveat","Undecoded bytes can be padding, jump tables, constants, or missed code. Function recognition and decompile success do not establish source or behavioral equivalence.");
  Files.writeString(root.resolve("reports/native-coverage.json"),gson.toJson(coverage));
  try(BufferedWriter out=Files.newBufferedWriter(root.resolve("reports/defined-strings.jsonl"))) {
   Gson compact=new Gson();DataIterator it=currentProgram.getListing().getDefinedData(true);
   while(it.hasNext()){Data d=it.next();if(!d.hasStringValue())continue;Object val=d.getValue();if(!(val instanceof String))continue;JsonObject j=new JsonObject();j.addProperty("rva","0x"+Long.toHexString(d.getAddress().subtract(base)));j.addProperty("value",(String)val);out.write(compact.toJson(j)+"\n");}
  }
  println("Final functions="+funcs+"; repaired="+r.decompileCompleted()+"; decoded bytes="+instructionBytes);
 }
}
