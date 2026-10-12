// Offline source processing only. Run with the pinned ICU4J jar and JDK 21+.
import com.ibm.icu.lang.UCharacter;
import com.ibm.icu.text.UnicodeSet;
import com.ibm.icu.text.UnicodeSetIterator;
import com.ibm.icu.util.ULocale;
import com.ibm.icu.util.VersionInfo;
import java.io.*;
import java.nio.charset.StandardCharsets;
import java.util.*;

class IcuBridge {
    static String decode(String s) {
        return new String(Base64.getDecoder().decode(s), StandardCharsets.UTF_8);
    }
    static String encode(String s) {
        return Base64.getEncoder().encodeToString(s.getBytes(StandardCharsets.UTF_8));
    }
    public static void main(String[] args) throws Exception {
        System.out.println("VERSION\t" + VersionInfo.ICU_VERSION + "\t" + UCharacter.getUnicodeVersion());
        var reader = new BufferedReader(new InputStreamReader(System.in, StandardCharsets.UTF_8));
        String line;
        int id = 0;
        while ((line = reader.readLine()) != null) {
            try {
                String[] parts = line.split("\t", -1);
                var results = new ArrayList<String>();
                if (parts[0].equals("SET") && parts.length == 2) {
                    var set = new UnicodeSet(decode(parts[1]), UnicodeSet.IGNORE_SPACE);
                    var it = new UnicodeSetIterator(set);
                    while (it.next()) {
                        String form = it.getString();
                        // UTF-8 must never replace an unpaired surrogate silently.
                        if (form.isEmpty() || form.codePoints().anyMatch(cp -> cp >= 0xD800 && cp <= 0xDFFF))
                            throw new IllegalArgumentException("empty/non-scalar UnicodeSet item");
                        results.add(encode(form));
                    }
                } else if (parts[0].equals("CASE") && parts.length == 3) {
                    var locale = new ULocale(decode(parts[1]));
                    String form = decode(parts[2]);
                    results.add(encode(UCharacter.toLowerCase(locale, form)));
                    results.add(encode(UCharacter.toUpperCase(locale, form)));
                    results.add(encode(UCharacter.toTitleCase(locale, form, null)));
                } else {
                    throw new IllegalArgumentException("invalid operation");
                }
                System.out.println(id + "\tOK\t" + String.join(",", results));
            } catch (IllegalArgumentException e) {
                System.out.println(id + "\tERROR\t" + encode(e.getMessage()));
            }
            id++;
        }
    }
}
