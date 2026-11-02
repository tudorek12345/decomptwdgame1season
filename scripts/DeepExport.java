// Targeted evidence export; does not change the program.
// @category GameRE
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import ghidra.program.model.pcode.PcodeOp;
import com.google.gson.*;
import java.nio.file.*;
import java.nio.charset.StandardCharsets;
import java.util.*;
import java.util.regex.Pattern;

public class DeepExport extends GhidraScript {
    private final Gson gson = new GsonBuilder().setPrettyPrinting().create();
    private String rva(Address address) {
        if (!address.getAddressSpace().equals(currentProgram.getImageBase().getAddressSpace())) return null;
        return "0x" + Long.toHexString(address.subtract(currentProgram.getImageBase()));
    }
    private JsonObject location(Address address) {
        JsonObject out = new JsonObject();
        out.addProperty("address", address.toString());
        out.addProperty("rva", rva(address));
        return out;
    }
    private JsonObject functionInfo(Function f) {
        JsonObject out = location(f.getEntryPoint());
        out.addProperty("name", f.getName(true));
        out.addProperty("signature", f.getSignature().toString());
        out.addProperty("calling_convention", f.getCallingConventionName());
        out.addProperty("body_address_count", f.getBody().getNumAddresses());
        out.addProperty("external", f.isExternal());
        out.addProperty("thunk", f.isThunk());
        return out;
    }
    private JsonObject referenceInfo(Reference ref) {
        JsonObject out = new JsonObject();
        out.add("from", location(ref.getFromAddress()));
        out.add("to", location(ref.getToAddress()));
        out.addProperty("type", ref.getReferenceType().toString());
        Function source = currentProgram.getFunctionManager().getFunctionContaining(ref.getFromAddress());
        if (source != null) out.addProperty("source_function", source.getName(true));
        return out;
    }
    private Pattern pattern(JsonObject cfg, String key) {
        String value = cfg.get(key).getAsString();
        return value.isEmpty() ? null : Pattern.compile(value);
    }
    @Override public void run() throws Exception {
        if (currentProgram == null || getScriptArgs().length != 1)
            throw new IllegalArgumentException("Usage: DeepExport.java absolute-request.json");
        JsonObject cfg = JsonParser.parseString(Files.readString(Path.of(getScriptArgs()[0]))).getAsJsonObject();
        Path target = Path.of(cfg.get("output").getAsString());
        if (Files.exists(target)) throw new IllegalArgumentException("Output already exists: " + target);
        Pattern names = pattern(cfg, "name_regex"), strings = pattern(cfg, "string_regex");
        int maxSelected = cfg.get("max_selected").getAsInt();
        int maxInstructions = cfg.get("max_instructions").getAsInt();
        int timeout = cfg.get("decompile_timeout").getAsInt();
        if (maxSelected < 1 || maxSelected > 200 || maxInstructions < 1 || maxInstructions > 20000 || timeout < 1 || timeout > 300)
            throw new IllegalArgumentException("Invalid export limits");
        Set<Address> requested = new HashSet<>();
        JsonArray unresolved = new JsonArray();
        for (JsonElement value : cfg.getAsJsonArray("rvas")) {
            long offset = Long.parseUnsignedLong(value.getAsString().replaceFirst("^0[xX]", ""), 16);
            Address address = currentProgram.getImageBase().add(offset);
            Function f = currentProgram.getFunctionManager().getFunctionContaining(address);
            if (f == null) unresolved.add(value.getAsString());
            else requested.add(f.getEntryPoint());
        }
        JsonObject report = new JsonObject();
        report.addProperty("schema_version", 1);
        report.addProperty("program", currentProgram.getName());
        report.addProperty("sha256", currentProgram.getExecutableSHA256());
        report.addProperty("image_base", currentProgram.getImageBase().toString());
        report.addProperty("language", currentProgram.getLanguageID().toString());
        report.addProperty("compiler", currentProgram.getCompilerSpec().getCompilerSpecID().toString());
        report.addProperty("executed_target", false);
        report.addProperty("graph_scope", "Ghidra-recognized references only; indirect/runtime targets may be missing");
        report.add("unresolved_rvas", unresolved);
        JsonArray stringRows = new JsonArray();
        int stringCount = 0;
        DataIterator dataIt = currentProgram.getListing().getDefinedData(true);
        while (dataIt.hasNext()) {
            monitor.checkCancelled();
            Data data = dataIt.next();
            if (!data.hasStringValue()) continue;
            Object value = data.getValue();
            if (!(value instanceof String)) continue;
            String text = (String)value;
            stringCount++;
            boolean match = strings != null && strings.matcher(text).find();
            JsonArray refs = new JsonArray();
            ReferenceIterator it = currentProgram.getReferenceManager().getReferencesTo(data.getAddress());
            int refCount = 0;
            while (it.hasNext()) {
                Reference ref = it.next();
                if (refCount++ < 64) refs.add(referenceInfo(ref));
                if (match) {
                    Function f = currentProgram.getFunctionManager().getFunctionContaining(ref.getFromAddress());
                    if (f != null) requested.add(f.getEntryPoint());
                }
            }
            if (stringRows.size() < 20000) {
                JsonObject row = location(data.getAddress());
                row.addProperty("value", text.substring(0, Math.min(text.length(), 4096)));
                row.addProperty("value_truncated", text.length() > 4096);
                row.addProperty("matched_selector", match);
                row.add("references_to", refs);
                row.addProperty("references_truncated", refCount > 64);
                stringRows.add(row);
            }
        }
        report.add("defined_strings", stringRows);
        report.addProperty("defined_string_count", stringCount);
        report.addProperty("strings_truncated", stringCount > stringRows.size());
        JsonArray index = new JsonArray();
        List<Function> selected = new ArrayList<>();
        int functionCount = 0, matches = 0;
        FunctionIterator fi = currentProgram.getFunctionManager().getFunctions(true);
        while (fi.hasNext()) {
            monitor.checkCancelled();
            Function f = fi.next();
            functionCount++;
            if (index.size() < 100000) index.add(functionInfo(f));
            boolean match = requested.contains(f.getEntryPoint()) || (names != null && names.matcher(f.getName(true)).find());
            if (match && !f.isExternal()) {
                matches++;
                if (selected.size() < maxSelected) selected.add(f);
            }
        }
        report.add("function_index", index);
        report.addProperty("enumerated_function_count", functionCount);
        report.addProperty("function_index_truncated", functionCount > index.size());
        report.addProperty("matched_function_count", matches);
        report.addProperty("selection_truncated", matches > selected.size());
        JsonArray details = new JsonArray();
        DecompInterface decompiler = new DecompInterface();
        try {
            if (!selected.isEmpty() && !decompiler.openProgram(currentProgram))
                throw new IllegalStateException("Decompiler failed to open program: " + decompiler.getLastMessage());
            for (Function f : selected) {
                monitor.checkCancelled();
                JsonObject row = functionInfo(f);
                JsonArray incoming = new JsonArray();
                ReferenceIterator ri = currentProgram.getReferenceManager().getReferencesTo(f.getEntryPoint());
                int count = 0;
                while (ri.hasNext()) { Reference ref = ri.next(); if (count++ < 1000) incoming.add(referenceInfo(ref)); }
                row.add("entry_references_to", incoming);
                row.addProperty("entry_references_truncated", count > 1000);
                JsonArray instructions = new JsonArray();
                InstructionIterator ii = currentProgram.getListing().getInstructions(f.getBody(), true);
                while (ii.hasNext() && instructions.size() < maxInstructions) {
                    Instruction instruction = ii.next();
                    JsonObject ins = location(instruction.getAddress());
                    ins.addProperty("assembly", instruction.toString());
                    ins.addProperty("bytes", HexFormat.of().formatHex(instruction.getBytes()));
                    JsonArray pcode = new JsonArray();
                    for (PcodeOp op : instruction.getPcode()) pcode.add(op.toString());
                    ins.add("raw_pcode", pcode);
                    JsonArray outgoing = new JsonArray();
                    for (Reference ref : instruction.getReferencesFrom()) outgoing.add(referenceInfo(ref));
                    ins.add("references_from", outgoing);
                    instructions.add(ins);
                }
                row.add("instructions", instructions);
                row.addProperty("instructions_truncated", ii.hasNext());
                DecompileResults result = decompiler.decompileFunction(f, timeout, monitor);
                row.addProperty("decompile_completed", result.decompileCompleted());
                row.addProperty("decompile_message", result.getErrorMessage());
                if (result.decompileCompleted() && result.getDecompiledFunction() != null) {
                    String code = result.getDecompiledFunction().getC();
                    row.addProperty("pseudocode", code.substring(0, Math.min(code.length(), 200000)));
                    row.addProperty("pseudocode_truncated", code.length() > 200000);
                }
                details.add(row);
            }
        } finally { decompiler.dispose(); }
        report.add("selected_functions", details);
        Files.writeString(target, gson.toJson(report), StandardCharsets.UTF_8, StandardOpenOption.CREATE_NEW);
        println("DeepExport saved " + target + "; selected=" + details.size() + "; indexed=" + index.size());
    }
}
