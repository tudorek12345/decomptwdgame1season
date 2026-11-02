// Read-only discovery audit: direct calls into .text without a recognized function.
// @category GameRE
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import com.google.gson.*;
import java.nio.file.*;
import java.util.*;
public class ScanUnownedCalls extends GhidraScript {
 public void run() throws Exception {
  Address base=currentProgram.getImageBase();AddressSet text=new AddressSet(base.add(0x1000),base.add(0x57d72b));
  InstructionIterator instructions=currentProgram.getListing().getInstructions(text,true);
  TreeMap<Long,JsonArray> targets=new TreeMap<>();int calls=0,interior=0;
  while(instructions.hasNext()) {
   Instruction i=instructions.next();if(!i.getFlowType().isCall())continue;
   for(Address target:i.getFlows()){
    if(!text.contains(target))continue;calls++;
    if(getFunctionAt(target)!=null)continue;
    if(getFunctionContaining(target)!=null){interior++;continue;}
    targets.computeIfAbsent(target.subtract(base),key->new JsonArray()).add("0x"+Long.toHexString(i.getAddress().subtract(base)));
   }
  }
  JsonArray candidates=new JsonArray();
  for(Map.Entry<Long,JsonArray> e:targets.entrySet()){
   JsonObject row=new JsonObject();row.addProperty("rva","0x"+Long.toHexString(e.getKey()));row.add("call_sites",e.getValue());
   row.addProperty("first_byte",currentProgram.getMemory().getByte(base.add(e.getKey()))&255);row.addProperty("evidence","Direct CALL from saved Ghidra instruction; no containing function");candidates.add(row);
  }
  JsonObject report=new JsonObject();report.addProperty("direct_calls_into_text",calls);report.addProperty("calls_into_existing_function_interiors",interior);report.add("unowned_targets",candidates);
  Files.writeString(Path.of(getScriptArgs()[0]),new GsonBuilder().setPrettyPrinting().create().toJson(report));
  println("Unowned direct-call targets: "+candidates.size());
 }
}
