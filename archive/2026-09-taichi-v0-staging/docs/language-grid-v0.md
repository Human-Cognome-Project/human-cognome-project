# LANGUAGE GRID v0 — the full Kaikki body (P's first step)

**Plan line (data-plan §7 rule 2):** serves §3 (extension to new languages: a new language = a new drain, not a new design) + §7.5 (Kaikki is the ONLY admitted new-data lane). Established 2026-09-01 on P's correction that this was the FIRST step.

## Purpose — P's validity prediction (2026-09-01, the test this grid exists to run)
> *"If we are in any way pursuing something valid, the full Kaikki body should form very complex but stable patterns that everything else will link to."*

The grid is not inventory — it is the test roster. The full linguistic set drains into the substrate, the field settles, and validity is read from whether **complex-but-stable patterns form** and become the thing all later matter links to. "Stable" is measurable today: the persistence criterion (closed/open by persistence-across-ticks, `engine/timestep/persistence_criterion_v0.py`) is the instrument; "complex" is the observable-arrow metric the seam already carries. Frequency stays emergent — never fed.

## The grid, in numbers (fetched live 2026-09-01: kaikki.org/dictionary/ + hcp_english.translations)
- **Kaikki per-language index: 461 languages, 12,838,823 senses** (index lists languages above ~500 senses).
- **All-languages-combined extract: 12,999,257 senses** — the sub-threshold long tail (160,434 senses) exists only in the combined download; the FULL body means the combined extract, not the sum of listed pages.
- **P's own data already names the full set: `hcp_english.translations` (314,497 rows) points at 3,924 distinct languages** — 3,505 of them below Kaikki's listing threshold. The multilingual bridge is IN the data; the grid confirms it rather than inventing it.
- Size tiers (listed languages): ≥1M senses: 2 · 100K–1M: 21 · 10K–100K: 68 · 1K–10K: 239 · <1K: 131.

## Current substrate state
- Drained: **English only** — `source_wiktionary.wiktextract_raw` 1,454,988 rows (all `lang_code=en`) → `source_english` → `hcp_english`, scoped at load to single-word blocks (see Loaded-extract condition above; the row-vs-senses delta is that scoping, not staleness).
- Pipeline exists end-to-end (data-plan §3): raw drain → source db → atomized hcp_* riding the universal AA byte layer (all-Unicode by construction). Each new language = the same drain run on its extract.

## Loaded-extract condition (P explaining state, 2026-09-01 — not a rule)
The English extract's 1.45M rows vs kaikki's 1.78M listed senses = the original work's SINGLE-WORD-BLOCK scoping (higher-order/multi-word forms not drained), not vintage — the Kaikki data is likely identical today; no refresh implied. Higher-order forms stay open: add on P's word, or watch whether they settle on their own. Scoping of each future drain = P's call at drain time.

## Drain order — P decides; three orderings on offer
1. **Size-descending** (Latin → Spanish → Italian → German → …): biggest mass first, patterns condense soonest.
2. **Bridge-density** (what English data already points at most): Spanish 9,771 · Finnish 9,764 · Russian 9,499 · German 9,488 · French 7,301 — links activate fastest.
3. **Contrast set** (near-English + agglutinative + unspaced, e.g. German + Finnish + Chinese): maximal early partition-contrast for the emergence questions.
These are orderings of the SAME full set — nothing is curated out (scale ruling ~1400). Drains fire language-by-language on P's word.

## Machine-readable grid
`docs/language-grid-v0.json` — every language (union of both views, 3,966 names): kaikki senses + English-bridge link count. Kaikki paths ride in `scratchpad` fetch; per-language extracts at `kaikki.org/dictionary/<Language>/`.

## Appendix — the 461 Kaikki-listed languages (senses)

**English** 1,780,480 · **Latin** 1,008,452 · **Spanish** 874,006 · **Italian** 719,427
**German** 631,714 · **Portuguese** 525,631 · **Russian** 492,165 · **French** 458,908
**Chinese** 388,964 · **Swedish** 345,936 · **Finnish** 309,778 · **Polish** 264,993
**Galician** 242,908 · **Japanese** 236,443 · **Catalan** 228,179 · **Dutch** 189,740
**Latvian** 172,685 · **Romanian** 148,024 · **Esperanto** 137,337 · **Greek** 112,866
**Mandarin** 112,609 · **Translingual** 106,957 · **Arabic** 100,111 · **Old English** 93,635
**Serbo-Croatian** 91,765 · **Hungarian** 90,518 · **Norwegian Bokmål** 89,837 · **Ancient Greek** 89,814
**Czech** 85,122 · **Korean** 81,150 · **Ukrainian** 80,952 · **Macedonian** 76,643
**Bulgarian** 74,690 · **Norwegian Nynorsk** 72,240 · **Middle English** 68,869 · **Danish** 67,628
**Vietnamese** 61,951 · **Turkish** 59,828 · **Hindi** 57,548 · **Indonesian** 55,669
**Tagalog** 55,516 · **Irish** 51,591 · **Asturian** 42,433 · **Welsh** 36,000
**Lithuanian** 34,608 · **Albanian** 33,063 · **Icelandic** 32,797 · **Georgian** 30,851
**Sanskrit** 30,742 · **Armenian** 29,246 · **Telugu** 28,646 · **Tamil** 28,544
**Persian** 28,087 · **Thai** 27,676 · **Navajo** 25,487 · **Cebuano** 25,261
**Gothic** 25,149 · **Scottish Gaelic** 24,377 · **Swahili** 23,389 · **Hebrew** 23,375
**Azerbaijani** 22,794 · **Ido** 22,285 · **Maltese** 22,203 · **Malay** 21,818
**Slovak** 21,453 · **Basque** 20,054 · **Urdu** 17,754 · **Oromo** 17,606
**Punjabi** 17,286 · **Malayalam** 16,913 · **Estonian** 16,768 · **Kazakh** 15,994
**Yiddish** 15,815 · **Old Norse** 15,687 · **Bengali** 15,641 · **Khmer** 15,276
**Ottoman Turkish** 14,931 · **Burmese** 14,448 · **Pannonian Rusyn** 13,722 · **Pali** 12,930
**Old Armenian** 12,880 · **Faroese** 12,799 · **Norman** 12,363 · **Tarifit** 12,306
**Assyrian Neo-Aramaic** 11,943 · **Old French** 11,026 · **Afrikaans** 10,945 · **Ingrian** 10,836
**Luxembourgish** 10,768 · **Cantonese** 10,633 · **Belarusian** 10,406 · **Gujarati** 9,995
**Slovene** 9,503 · **Cornish** 9,443 · **Central Bikol** 9,077 · **Old Irish** 8,767
**Mongolian** 8,567 · **Egyptian** 8,551 · **Classical Syriac** 8,411 · **Ladin** 8,399
**Occitan** 8,159 · **Northern Kurdish** 8,157 · **Yoruba** 8,090 · **Marathi** 8,054
**Proto-Germanic** 7,727 · **West Circassian** 7,710 · **Javanese** 7,679 · **Old Polish** 7,362
**Northern Sami** 7,350 · **Proto-West Germanic** 7,310 · **Scots** 7,220 · **Makasar** 7,219
**Proto-Slavic** 7,171 · **Hawaiian** 6,458 · **Livonian** 6,385 · **Ahtna** 6,284
**Assamese** 6,018 · **Old Church Slavonic** 5,896 · **Khiamniungan Naga** 5,807 · **Aromanian** 5,693
**Cimbrian** 5,642 · **Tajik** 5,631 · **Crimean Tatar** 5,576 · **Venetan** 5,555
**Old Czech** 5,529 · **Middle French** 5,421 · **Plautdietsch** 5,387 · **Manx** 5,379
**Māori** 5,280 · **Bashkir** 5,258 · **Uyghur** 5,244 · **Classical Nahuatl** 5,194
**Uzbek** 5,094 · **Yakut** 4,912 · **Zulu** 4,864 · **West Frisian** 4,830
**Votic** 4,743 · **Lower Sorbian** 4,689 · **Tibetan** 4,660 · **Kyrgyz** 4,581
**Odia** 4,565 · **Sicilian** 4,558 · **Volapük** 4,477 · **South Levantine Arabic** 4,358
**Interlingua** 4,356 · **Malagasy** 4,316 · **Middle Dutch** 4,311 · **Old High German** 4,243
**Proto-Finnic** 4,235 · **Sundanese** 4,199 · **Kashubian** 4,154 · **Lower Tanana** 4,108
**Laz** 4,083 · **Coptic** 4,002 · **Kannada** 3,941 · **Quechua** 3,897
**Zhuang** 3,801 · **Ushojo** 3,789 · **Xhosa** 3,699 · **Emilian** 3,689
**Silesian** 3,687 · **Northern Yukaghir** 3,598 · **Old Javanese** 3,595 · **Amharic** 3,537
**Bavarian** 3,506 · **Palula** 3,390 · **Kashmiri** 3,355 · **Lao** 3,330
**Yola** 3,330 · **Veps** 3,224 · **Hijazi Arabic** 3,220 · **Ladino** 3,185
**Manchu** 3,182 · **Sumerian** 3,165 · **Aramaic** 3,161 · **Shan** 3,140
**Pennsylvania German** 3,139 · **East Central German** 3,125 · **Nepali** 3,087 · **Greenlandic** 3,034
**Chichewa** 3,025 · **Low German** 3,014 · **Walloon** 3,010 · **Prakrit** 2,952
**Alemannic German** 2,933 · **Hokkien** 2,912 · **Franco-Provençal** 2,877 · **Tocharian B** 2,857
**Moksha** 2,830 · **Limburgish** 2,826 · **Old Saxon** 2,785 · **Moroccan Arabic** 2,783
**Hunsrik** 2,748 · **Hausa** 2,711 · **Breton** 2,624 · **Ligurian** 2,612
**Romansh** 2,603 · **Friulian** 2,591 · **Ye'kwana** 2,585 · **Hiligaynon** 2,547
**Ainu** 2,547 · **Proto-Indo-European** 2,533 · **Saterland Frisian** 2,523 · **Dhivehi** 2,511
**Haitian Creole** 2,500 · **Unami** 2,479 · **Turkmen** 2,461 · **Afar** 2,456
**Tashelhit** 2,408 · **Sranan Tongo** 2,356 · **Tok Pisin** 2,348 · **Old Dutch** 2,343
**Fula** 2,302 · **Old Tupi** 2,296 · **Cherokee** 2,295 · **Southern Altai** 2,288
**Eastern Mari** 2,279 · **Tatar** 2,235 · **Akkadian** 2,224 · **Middle High German** 2,217
**Kapampangan** 2,215 · **Khalaj** 2,209 · **Pashto** 2,158 · **Sindhi** 2,148
**Mokilese** 2,123 · **Kumyk** 2,116 · **Sirenik** 2,080 · **Ojibwe** 2,067
**Udmurt** 2,042 · **Marshallese** 2,041 · **Tangut** 2,026 · **Proto-Samic** 2,020
**Ilocano** 2,017 · **Old Swedish** 2,012 · **Aragonese** 2,006 · **Kildin Sami** 1,978
**Old Galician-Portuguese** 1,953 · **Woiwurrung** 1,950 · **Proto-Celtic** 1,932 · **Egyptian Arabic** 1,904
**Lingala** 1,903 · **Neapolitan** 1,885 · **Upper Sorbian** 1,885 · **Balinese** 1,877
**Swazi** 1,853 · **Central Nahuatl** 1,853 · **Chuvash** 1,827 · **Vilamovian** 1,820
**Romagnol** 1,814 · **Tày** 1,790 · **Senhaja de Srair** 1,782 · **Abkhaz** 1,781
**Ternate** 1,781 · **Talysh** 1,755 · **Akan** 1,731 · **Sardinian** 1,722
**Mon** 1,717 · **North Frisian** 1,717 · **Salar** 1,714 · **Gagauz** 1,708
**Central Kurdish** 1,706 · **East Circassian** 1,662 · **Komi-Zyrian** 1,646 · **Old Spanish** 1,633
**Chavacano** 1,600 · **Iban** 1,578 · **Tausug** 1,568 · **Norwegian** 1,564
**Chickasaw** 1,560 · **Jeju** 1,513 · **Ubykh** 1,480 · **Konkani** 1,474
**Kikuyu** 1,470 · **Sinhalese** 1,461 · **Proto-Turkic** 1,460 · **Northern Mansi** 1,447
**Mauritian Creole** 1,432 · **Kabuverdianu** 1,429 · **Erzya** 1,423 · **Istriot** 1,404
**White Hmong** 1,399 · **Karelian** 1,397 · **Phuthi** 1,396 · **Dalmatian** 1,392
**Okinawan** 1,391 · **S'gaw Karen** 1,356 · **Choctaw** 1,353 · **Northern Altai** 1,330
**Zazaki** 1,328 · **Slovincian** 1,327 · **Chechen** 1,311 · **Sikkimese** 1,309
**Central Franconian** 1,297 · **Papiamentu** 1,295 · **Mapudungun** 1,292 · **Ugaritic** 1,289
**Romani** 1,288 · **Somali** 1,284 · **Sylheti** 1,261 · **San Juan Quiahije Chatino** 1,255
**Fijian** 1,233 · **Tokelauan** 1,227 · **Old East Slavic** 1,217 · **Garo** 1,214
**Maranao** 1,213 · **Middle Korean** 1,179 · **Lule Sami** 1,160 · **Corsican** 1,158
**Magahi** 1,148 · **Paraguayan Guarani** 1,144 · **North Levantine Arabic** 1,144 · **Manipuri** 1,116
**Piedmontese** 1,112 · **Ossetian** 1,105 · **Tigrinya** 1,087 · **Kankanaey** 1,086
**Gawar-Bati** 1,083 · **Mingrelian** 1,071 · **Tuvan** 1,059 · **Ket** 1,054
**Tumbuka** 1,053 · **Macanese** 1,048 · **West Makian** 1,031 · **Middle Irish** 1,026
**Lombard** 1,025 · **Santali** 1,024 · **Gulf Arabic** 1,014 · **Bambara** 1,012
**Lakota** 1,010 · **Fala** 1,004 · **Mizo** 997 · **Old Anatolian Turkish** 996
**Chuukese** 990 · **Inupiaq** 984 · **Middle Welsh** 977 · **Ludian** 977
**Amis** 977 · **Proto-Indo-Iranian** 974 · **Old Slovak** 957 · **Karao** 947
**Lushootseed** 934 · **Tooro** 929 · **Old Georgian** 924 · **Kavalan** 916
**Ge'ez** 893 · **Jamaican Creole** 892 · **Gun** 869 · **Proto-Malayo-Polynesian** 864
**Ewe** 864 · **Old Ruthenian** 859 · **Proto-Italic** 856 · **Pohnpeian** 839
**Urak Lawoi'** 836 · **Kalmyk** 828 · **Nivkh** 828 · **Louisiana Creole** 827
**Gamilaraay** 822 · **Waray-Waray** 821 · **Lü** 821 · **Proto-Bantu** 820
**Ghomala'** 817 · **Chagatai** 813 · **Mohawk** 809 · **Samoan** 807
**Proto-Samoyedic** 807 · **Wolof** 804 · **Central Huasteca Nahuatl** 794 · **Proto-Brythonic** 794
**Aghwan** 792 · **Rwanda-Rundi** 791 · **Livvi** 789 · **Even** 783
**Wolaytta** 780 · **Old Occitan** 770 · **Hamer-Banna** 768 · **Yucatec Maya** 767
**Võro** 762 · **Acehnese** 754 · **Proto-Uralic** 750 · **Tetum** 748
**Murui Huitoto** 747 · **Evenki** 746 · **Sassarese** 743 · **Mirandese** 739
**Sakizaya** 733 · **Ingush** 732 · **Antigua and Barbuda Creole English** 728 · **Dupaningan Agta** 728
**Central Atlas Tamazight** 723 · **Old Frisian** 720 · **Cubeo** 708 · **Southern Ndebele** 704
**Sotho** 699 · **Old Japanese** 689 · **Betawi** 687 · **Eastern Huasteca Nahuatl** 682
**Skolt Sami** 680 · **Lindu** 677 · **Latgalian** 674 · **Tarantino** 672
**Buryat** 672 · **Nanai** 671 · **Mariupol Greek** 668 · **Nheengatu** 666
**Polabian** 663 · **Atayal** 662 · **Tundra Nenets** 660 · **Tlingit** 656
**Yogad** 654 · **Yurok** 652 · **Bourguignon** 652 · **Inari Sami** 648
**Baluchi** 644 · **Avar** 640 · **Kalasha** 635 · **Old Persian** 634
**Mycenaean Greek** 633 · **Old Uyghur** 627 · **Umbrian** 621 · **Chungli Ao** 619
**Tetelcingo Nahuatl** 617 · **Shona** 612 · **Nupe** 610 · **Achang** 606
**Hanunoo** 592 · **Taos** 581 · **Tocharian A** 580 · **Anguthimri** 576
**Proto-Ryukyuan** 571 · **Nǀuu** 568 · **Cahuilla** 567 · **Ghomara** 566
**Yao (Africa)** 557 · **Proto-Japonic** 557 · **Saanich** 556 · **Inuktitut** 553
**Etruscan** 551 · **Pite Sami** 548 · **Proto-Balto-Slavic** 546 · **Proto-Permic** 546
**Middle Armenian** 546 · **Khakas** 544 · **Northern Ndebele** 541 · **Kurtöp** 539
**Tai Nüa** 539 · **Sambali** 537 · **Svan** 533 · **Bats** 531
**Hittite** 529 · **Brunei Malay** 526 · **Proto-Sino-Tibetan** 524 · **Western Apache** 522
**Dakota** 520 · **Bangi** 517 · **Elfdalian** 517 · **Mbya Guarani** 513
**Proto-Iranian** 511 · **Nogai** 507 · **K'iche'** 505 · **Old Turkic** 504
**Cypriot Arabic** 503
