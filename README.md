**Welcome to the Human Cognome Project (HCP) 🧠**
Hi there! We are so glad you found us. The Human Cognome Project is an ambitious, open-source effort to build a "brain" for AI—one that understands the world through the same physical and relational laws that humans do.

By using the Open 3D Engine (O3DE) and PhysX 5, we aren't just teaching an AI to talk; we are teaching it how things "connect" in a physical world. Whether you are a veteran C++ developer or just starting your journey into data processing, there is a place for you here.

**🛠 Getting Started (The Onboarding Map)**
Don't worry if this looks like a lot at first! This guide will walk you through setting up your own local "cognome" environment step-by-step.

**Step 1: Gather Your Tools**
The HCP engine is powerful, which means it needs a few specialized tools to run. Think of these as the "engine parts" you need to install on your computer:

O3DE SDK: Our 3D simulation framework (the body of the engine).

PhysX 5 SDK: This handles the "physics" of how ideas interact.

PostgreSQL: Our memory bank where long-term data is stored.

Python 3.10+: The "glue" we use for scripts and data processing.

**Step 2: Grab the Code**
Download the project files to your computer using Git:

git clone --recursive https://github.com/Human-Cognome-Project/human-cognome-project.git
cd human-cognome-project

**Step 3: Set Up Your Memory (Database)**
The engine needs a place to store its thoughts. We use two types of databases:

**PostgreSQL (The Shard Vault):**
Run these two simple scripts to prepare your database.
./scripts/setup_db.sh
./scripts/run_migrations.sh

**LMDB (The Vocabulary):**
Our AI needs to learn words! This script compiles 1.4 million tokens into a fast-access format.
python3 scripts/compile_vocab_lmdb.py --input data/raw_vocab.txt --output data/vocab.lmdb/

Pro-tip: Run our new "verify_lmdb.py" script afterward to make sure everything was saved correctly!

**Step 4: Build the Engine**
This is the magic moment where the code becomes a running program.

**If you are on a Desktop (Windows/Linux):**
cmake -B build/pc -S .
cmake --build build/pc --target HCPWorkstation

**If you are on a Mobile Server or ARM64 (Experimental):**
We support "Headless" mode for low-power devices!
cmake -B build/arm -S . -DLY_HEADLESS=ON -DLY_CPU_ONLY=ON
cmake --build build/arm --target HCPWorkstation

**🤝 Your First Contribution**
We know joining a big project can be intimidating, but we promise we don't bite! Here is how to dive in:

Find a "Good First Issue": We have tasks like the HTML Text Extractor or Dead Code Cleanup that are perfect for getting your feet wet.

Ask for Help: If you get stuck, reach out to VagariesOfFate on GitHub or Discord. We’ve all been the "new person" before.

Documentation Matters: Notice a typo in this README? That’s a valid contribution! Feel free to fix it.

**📁 Where Everything Lives**
/hcp-engine: The "brain" (C++ code).

/scripts: The "helpers" (Python tools for the database).

/data: The "knowledge" (Raw and compiled datasets).

/docs: The "blueprints" (Technical explanations of how it all works).

Thank you for being part of the future of decentralized cognition. Let's build something amazing together!
