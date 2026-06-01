# PBM Reference Systems Research

Research conducted: 2026-02-08

## Executive Summary

For building a prototype PBM reference system that handles heterogeneous document types and generates discoverable metadata, existing library science and NLP standards provide proven foundations.

## Key Standards and Tools

### Document Metadata - Dublin Core
- 15 core elements (title, creator, date, subject, description, etc.)
- Proven standard, widely understood, extensible
- Store templates in core at AA.AE.* (document type templates)

### Document Parsing - Apache Tika
- Handles 1000+ file types (PDF, DOC, HTML, etc.)
- Single unified API, 18+ years proven in production
- Extracts both text and metadata automatically

### Concept Extraction - TextRank
- Unsupervised keyword extraction (no training data needed)
- Graph-based algorithm, surprisingly effective
- Fast, proven, good enough for prototype

### Entity Recognition - spaCy
- Fast, production-ready NER
- Identifies people, places, organizations, dates
- Pre-trained models available, Python library

### Concept Organization - SKOS
- W3C standard for taxonomies and controlled vocabularies
- Hierarchical concept relationships
- Multilingual support (aligns with HCP's y* name components)
- RDF-based, semantic web compatible

## Recommended Architecture

### Phase 1: Document Ingestion
```
Document Input → Type Detection (Tika)
                ↓
         Format Extraction
                ↓
    Schema Skeleton Inference
                ↓
     Normalized Metadata + Structured Content
```

### Phase 2: Concept Extraction
```
Extracted Text → NER (spaCy) + Entity Recognition
                ↓
    Keyword Extraction (TextRank)
                ↓
    Entity Linking (to v*/w*/x* or r*/s*/t*)
```

### Phase 3: PBM Generation with Metadata
```
Tokenized Content → PBM Structure (FPB entries)
                   ↓
            Metadata Attachment:
            - Dublin Core fields
            - Temporal reference (AA.AD.CE.*)
            - Entity references (v-z namespace tokens)
            - Extracted concepts/keywords
            - Document type template ID
                   ↓
        Store in z* (non-fiction) or u* (fiction)
```

### Phase 4: Discovery and Querying
```
User Query → Multiple pivot dimensions:
            - Temporal (year range)
            - Entities mentioned (people/places/things)
            - Concepts/keywords (SKOS hierarchy)
            - Document type
            - Subject classification
            - Structural similarity (PBM pattern matching)
```

## Document Type Templates

Each template defines:
- Parsing rules for structure extraction
- Scope boundaries (what's a meaningful unit for PBM generation)
- Metadata schema (Dublin Core + type-specific extensions)
- Source tracking (URL, archive date, version)

Common types to support:
- Books (chapters, sections, footnotes)
- Articles (title, abstract, sections, citations)
- Web pages (headers, content blocks, navigation)
- Social media (posts, threads, timestamps)
- Forums (threads, posts, quotes)
- Email (headers, body, threading)
- Code documentation (modules, functions, examples)
- Legal documents (sections, clauses, references)

## Key Advantages

1. **Heterogeneity-proof** - Handles diverse source types
2. **Unsupervised** - TextRank and spaCy need no custom training
3. **Scalable** - All components are production-proven
4. **Interpretable** - SKOS and RDF enable transparent concept relationships
5. **Evolvable** - Schema registry pattern prevents breaking changes
6. **Cross-linguistic** - SKOS multilingual support aligns with HCP design

## Implementation Priority

1. Define Dublin Core + HCP extensions for PBM metadata schema
2. Create document type templates for common sources
3. Build simple ingestion tool using these standards
4. Generate PBMs with rich metadata for discovery

## Full Research Details

See: `/root/.claude/projects/-home-patrick-gits-human-cognome-project/9beae90a-4c17-4dd3-8650-76764e01f85c.jsonl` - agent a779540 for complete research with sources and citations.

Key research areas covered:
- Document templating and schema standards (Dublin Core, TEI, Schema.org, MARC)
- Modern CMS approaches and schema composition
- Concept extraction and topic modeling (TextRank, YAKE, LLM-based approaches)
- NER and entity linking (spaCy, ReLiK, Stanford NER)
- Practical tools (Apache Tika, Python NLP libraries)
- Knowledge organization (SKOS, OWL, RDF)
- Knowledge graph construction patterns
- Multimodal document processing (DocLLM, layout-aware models)
- RAG systems for document question-answering
- Schema management and versioning (Confluent pattern)
