# Project Update from Patrick

This is the first update to the public git in a few weeks but that is a lack of concrete progress until this update, not a lack of work.

Early attempts to create a working English shard focused on trying to find a balance between the Token_IDs assigned and viable meaning structure.

After many false starts, the eventual decision was to return to what was discussed in the original data spec and assign a Token_ID to every possible form and relevant combination of words used in the english language and tie them together via layered common data points associated with each form.

The current English shard is a complete ingestion of the Kaikki.org/Wiktionary source data for modern english (en). This is expected to allow ingestion and reproduction of the majority of Project Gutenberg texts. Any words, names or other elements detected during ingestion will be added to the current shard, including misspellings and any other word forms presented.

Accompanying the English shard are 2 English document databases (fiction and non-fiction by library science code), and 6 language independent Entity databases. The Entity databases contain entries for literary and non-literary Proper Noun categories (Person, Place and Thing) to accurately represent the constellation of factors that cross reference Proper Noun defined elements in language.

Other documents explain the current naming schema for Token_IDs. This is somewhat loose for convenience but further refinement of the numbering schema is encouraged.

Additional language shards can be assembled at any point with cross link points available in the English database. Dialectic and/or archaic expansions of English are also encouraged.

Physics based resolution of documents will be aligned with the current db structure with a goal of 98% surface reconstruction with no significant loss of data integrity. When that is achieved, Gutenberg texts will be ingested as time allows but development on documents will pause to focus on NSM modeling.

Every effort will be made to update ToDo's and Issues but any contributor or agent is encouraged to inquire or be creative if an opportunity not listed is noted.
