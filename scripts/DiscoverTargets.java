// Add function starts only where registration or validated RTTI identifies a code pointer.
// @category GameRE
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import com.google.gson.*;
import java.nio.file.*;
public class DiscoverTargets extends GhidraScript {
 public void run() throws Exception {
  Path input=Path.of(getScriptArgs()[0]),output=Path.of(getScriptArgs()[1]);
  JsonArray results=new JsonArray();int created=0,known=0,interior=0,failed=0;
  for(JsonElement el:JsonParser.parseString(Files.readString(input)).getAsJsonArray()) {
   monitor.checkCancelled();JsonObject item=el.getAsJsonObject().deepCopy();String r=item.get("rva").getAsString();
   Address a=currentProgram.getImageBase().add(Long.parseLong(r.substring(2),16));
   Function f=getFunctionAt(a);
   if(f!=null){item.addProperty("status","existing");known++;}
   else if(getFunctionContaining(a)!=null){item.addProperty("status","inside_existing_function");item.addProperty("containing",getFunctionContaining(a).getName());interior++;}
   else {
    try {disassemble(a);f=createFunction(a,null);
     if(f!=null){item.addProperty("status","created");f.setComment("Function start identified by "+item.get("evidence").toString());created++;}
     else {item.addProperty("status","failed");failed++;}
    }catch(Exception e){item.addProperty("status","exception");item.addProperty("error",e.toString());failed++;}
   }
   results.add(item);
  }
  Files.writeString(output,new GsonBuilder().setPrettyPrinting().create().toJson(results));
  println("Discovery created="+created+" existing="+known+" interior="+interior+" failed="+failed);
  analyzeChanges(currentProgram);
 }
}
