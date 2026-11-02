// Full recognized-function export, deterministic resumable batches. @category GameRE
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import ghidra.program.model.data.DataTypeWriter;
import com.google.gson.*;
import java.nio.file.*;
import java.nio.charset.StandardCharsets;
import java.io.*;
import java.util.*;
import java.util.concurrent.*;
import java.util.concurrent.atomic.AtomicInteger;

public class ExportRecovery extends GhidraScript {
    final Gson gson = new Gson();
    String rva(Address a) { return "0x"+Long.toHexString(a.subtract(currentProgram.getImageBase())); }
    JsonObject info(Function f) {
        JsonObject j=new JsonObject(); j.addProperty("rva",rva(f.getEntryPoint()));
        j.addProperty("name",f.getName(true));j.addProperty("signature",f.getSignature().toString());
        j.addProperty("body_bytes",f.getBody().getNumAddresses());j.addProperty("thunk",f.isThunk());
        j.addProperty("calling_convention",f.getCallingConventionName());return j;
    }
    void batch(Path root, List<Function> fs, int number) throws Exception {
        String stem=String.format("batch_%04d",number);
        Path done=root.resolve(stem+".done.json"); if(Files.exists(done))return;
        DecompInterface di=new DecompInterface();di.openProgram(currentProgram);
        int good=0,bad=0;
        try(BufferedWriter cpp=Files.newBufferedWriter(root.resolve(stem+".cpp"));
            BufferedWriter records=Files.newBufferedWriter(root.resolve(stem+".jsonl"))) {
            cpp.write("// RAW GHIDRA PSEUDOCODE. Not a compilable translation unit.\n// Analysis image SHA256: "+currentProgram.getExecutableSHA256()+"\n// Image base: "+currentProgram.getImageBase()+"; see reports/capture-validation.json.\n");
            for(Function f:fs) {
                monitor.checkCancelled();JsonObject row=info(f);
                try {
                    DecompileResults res=di.decompileFunction(f,30,monitor);
                    boolean retried=false;
                    if(!res.decompileCompleted()) {di.flushCache();res=di.decompileFunction(f,120,monitor);retried=true;}
                    row.addProperty("retried",retried);row.addProperty("message",res.getErrorMessage());
                    if(res.decompileCompleted()&&res.getDecompiledFunction()!=null) {
                        String code=res.getDecompiledFunction().getC();row.addProperty("status","decompiled");
                        row.addProperty("characters",code.length());cpp.write("\n// RVA "+rva(f.getEntryPoint())+" | "+f.getName(true)+"\n"+code+"\n");good++;
                    } else {row.addProperty("status","failed");bad++;cpp.write("\n// FAILED RVA "+rva(f.getEntryPoint())+" "+f.getName(true)+"; see batch JSONL and text.asm\n");}
                } catch(Exception e) {row.addProperty("status","exception");row.addProperty("error",e.toString());bad++;}
                records.write(gson.toJson(row)+"\n"); records.flush();
            }
        } finally {di.dispose();}
        JsonObject summary=new JsonObject();summary.addProperty("functions",fs.size());summary.addProperty("decompiled",good);summary.addProperty("failed",bad);
        Files.writeString(done,gson.toJson(summary));println(stem+" complete "+good+" decompiled, "+bad+" failed");
    }
    public void run() throws Exception {
        Path root=Path.of(getScriptArgs()[0]);Files.createDirectories(root);
        Path bindings=Path.of(getScriptArgs()[1]);
        if(Files.exists(bindings))for(JsonElement element:JsonParser.parseString(Files.readString(bindings)).getAsJsonArray()) {
            JsonObject j=element.getAsJsonObject();
            Address a=currentProgram.getImageBase().add(Long.parseLong(j.get("function_rva").getAsString().substring(2),16));
            Function f=getFunctionAt(a);
            if(f!=null) {
                String name="lua_"+j.get("name").getAsString().replaceAll("[^A-Za-z0-9_]","_");
                if(f.getSymbol().getSource()==SourceType.DEFAULT) f.setName(name,SourceType.USER_DEFINED);
                f.setComment("Candidate Lua native binding from registration instruction pattern: "+j.get("name").getAsString()+". ABI inferred by Ghidra; not runtime validated.");
            }
        }
        Set<String> prior=new HashSet<>();
        if(getScriptArgs().length>2) for(String line:Files.readAllLines(Path.of(getScriptArgs()[2])))
            if(!line.isBlank())prior.add(JsonParser.parseString(line).getAsJsonObject().get("rva").getAsString());
        List<Function> all=new ArrayList<>();List<Function> excluded=new ArrayList<>();
        FunctionIterator it=currentProgram.getFunctionManager().getFunctions(true);
        while(it.hasNext()) {Function f=it.next();long off=f.getEntryPoint().subtract(currentProgram.getImageBase());
            if(off>=0x1000 && off<0x57d72c && !f.isExternal()) {if(!prior.contains(rva(f.getEntryPoint())))all.add(f);} else excluded.add(f);}
        try(BufferedWriter index=Files.newBufferedWriter(root.resolve("functions.jsonl"))) {
            for(int i=0;i<all.size();i++){JsonObject row=info(all.get(i));row.addProperty("file",String.format("batch_%04d.cpp",i/256));index.write(gson.toJson(row)+"\n");}
        }
        try(BufferedWriter out=Files.newBufferedWriter(root.resolve("excluded-functions.jsonl"))) {for(Function f:excluded)out.write(gson.toJson(info(f))+"\n");}
        try(BufferedWriter out=Files.newBufferedWriter(root.resolve("types.h"))) {new DataTypeWriter(currentProgram.getDataTypeManager(),out).write(monitor);}
        try(BufferedWriter out=Files.newBufferedWriter(root.resolve("symbols.jsonl"))) {
            SymbolIterator syms=currentProgram.getSymbolTable().getAllSymbols(true);
            while(syms.hasNext()){Symbol s=syms.next();JsonObject j=new JsonObject();j.addProperty("address",s.getAddress().toString());j.addProperty("name",s.getName(true));j.addProperty("kind",s.getSymbolType().toString());j.addProperty("source",s.getSource().toString());out.write(gson.toJson(j)+"\n");}
        }
        try(BufferedWriter out=Files.newBufferedWriter(root.resolve("text.asm"))) {
            InstructionIterator ins=currentProgram.getListing().getInstructions(new AddressSet(currentProgram.getImageBase().add(0x1000),currentProgram.getImageBase().add(0x57d72b)),true);
            while(ins.hasNext()){Instruction i=ins.next();out.write(rva(i.getAddress())+" "+HexFormat.of().formatHex(i.getBytes())+" "+i+"\n");}
        }
        ExecutorService pool=Executors.newFixedThreadPool(4);List<Future<?>> futures=new ArrayList<>();
        try {
            for(int start=0;start<all.size();start+=256){final int n=start/256;final List<Function> fs=new ArrayList<>(all.subList(start,Math.min(start+256,all.size())));
                futures.add(pool.submit(()->{try{batch(root,fs,n);}catch(Exception e){throw new RuntimeException(e);}}));}
            for(Future<?> f:futures)f.get();
        } finally {pool.shutdownNow();}
        JsonObject summary=new JsonObject();summary.addProperty("identified_text_functions",all.size());summary.addProperty("excluded_functions",excluded.size());
        summary.addProperty("analysis_sha256",currentProgram.getExecutableSHA256());summary.addProperty("complete",true);
        summary.addProperty("scope","All Ghidra-recognized .text functions; identification is not proof of exhaustive code coverage. Encrypted .bind excluded.");
        Files.writeString(root.resolve("export-summary.json"),gson.toJson(summary));
    }
}
