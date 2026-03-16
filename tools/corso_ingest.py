#!/usr/bin/env python3
"""
corso_ingest.py — Ingest a ScanTailor RAG export into a SCIENCE-RAG / Corso
vector store.

The ScanTailor CLI produces an export directory containing:
  manifest.json   — page metadata index (schema v1.0)
  page_000.png    — processed page images
  page_001.png
  ...

This script reads that directory and loads the pages into the vector store
backend of your choice (ChromaDB, Qdrant, or Weaviate).

Usage
-----
  python corso_ingest.py <export_dir>
      [--collection COLLECTION]
      [--backend   {chroma,qdrant,weaviate}]
      [--embed     {clip,none}]
      [--host      HOST]
      [--port      PORT]

Examples
--------
  # Quick local test (no embeddings, just metadata):
  python corso_ingest.py ./rag_output --backend chroma --embed none

  # Full CLIP-based image embedding into ChromaDB:
  python corso_ingest.py ./rag_output --backend chroma --embed clip

  # Qdrant on a remote host:
  python corso_ingest.py ./rag_output --backend qdrant --host 192.168.1.10 --port 6333

Dependencies (install only what you need)
------------------------------------------
  pip install chromadb               # ChromaDB backend
  pip install qdrant-client          # Qdrant backend
  pip install weaviate-client        # Weaviate backend
  pip install transformers torch     # CLIP embeddings
  pip install Pillow                 # Image loading for CLIP
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any


# ---------------------------------------------------------------------------
# Embedding helpers
# ---------------------------------------------------------------------------

def embed_clip(image_paths: list[Path]) -> list[list[float]]:
    """Return CLIP image embeddings for every path."""
    try:
        from PIL import Image
        import torch
        from transformers import CLIPProcessor, CLIPModel
    except ImportError as exc:
        sys.exit(
            f"CLIP embedding requires: pip install transformers torch Pillow\n{exc}"
        )

    model_name = "openai/clip-vit-base-patch32"
    print(f"Loading CLIP model '{model_name}' …")
    model = CLIPModel.from_pretrained(model_name)
    processor = CLIPProcessor.from_pretrained(model_name)
    model.eval()

    embeddings: list[list[float]] = []
    with torch.no_grad():
        for path in image_paths:
            img = Image.open(path).convert("RGB")
            inputs = processor(images=img, return_tensors="pt")
            feats = model.get_image_features(**inputs)
            # Normalise to unit length (cosine similarity)
            feats = feats / feats.norm(dim=-1, keepdim=True)
            embeddings.append(feats.squeeze().tolist())
    return embeddings


def embed_none(image_paths: list[Path]) -> list[list[float]]:
    """Dummy embedder — returns a zero vector (for metadata-only ingestion)."""
    return [[0.0] * 512 for _ in image_paths]


EMBEDDERS = {
    "clip": embed_clip,
    "none": embed_none,
}


# ---------------------------------------------------------------------------
# Backend helpers
# ---------------------------------------------------------------------------

def ingest_chroma(
    ids: list[str],
    embeddings: list[list[float]],
    metadatas: list[dict[str, Any]],
    collection: str,
    host: str,
    port: int,
) -> None:
    try:
        import chromadb
    except ImportError as exc:
        sys.exit(f"ChromaDB backend requires: pip install chromadb\n{exc}")

    if host and host not in ("localhost", "127.0.0.1"):
        client = chromadb.HttpClient(host=host, port=port)
    else:
        client = chromadb.Client()

    col = client.get_or_create_collection(
        name=collection,
        metadata={"hnsw:space": "cosine"},
    )
    col.add(ids=ids, embeddings=embeddings, metadatas=metadatas)
    print(f"  Upserted {len(ids)} documents into ChromaDB collection '{collection}'.")


def ingest_qdrant(
    ids: list[str],
    embeddings: list[list[float]],
    metadatas: list[dict[str, Any]],
    collection: str,
    host: str,
    port: int,
) -> None:
    try:
        from qdrant_client import QdrantClient
        from qdrant_client.models import Distance, VectorParams, PointStruct
    except ImportError as exc:
        sys.exit(f"Qdrant backend requires: pip install qdrant-client\n{exc}")

    client = QdrantClient(host=host or "localhost", port=port or 6333)
    dim = len(embeddings[0])

    if not client.collection_exists(collection):
        client.create_collection(
            collection_name=collection,
            vectors_config=VectorParams(size=dim, distance=Distance.COSINE),
        )

    points = [
        PointStruct(id=i, vector=emb, payload=meta)
        for i, (emb, meta) in enumerate(zip(embeddings, metadatas))
    ]
    client.upsert(collection_name=collection, points=points)
    print(f"  Upserted {len(points)} points into Qdrant collection '{collection}'.")


def ingest_weaviate(
    ids: list[str],
    embeddings: list[list[float]],
    metadatas: list[dict[str, Any]],
    collection: str,
    host: str,
    port: int,
) -> None:
    try:
        import weaviate
    except ImportError as exc:
        sys.exit(f"Weaviate backend requires: pip install weaviate-client\n{exc}")

    url = f"http://{host or 'localhost'}:{port or 8080}"
    client = weaviate.Client(url)
    class_name = collection.capitalize()

    if not client.schema.exists(class_name):
        client.schema.create_class({
            "class": class_name,
            "vectorizer": "none",
        })

    with client.batch as batch:
        for uid, emb, meta in zip(ids, embeddings, metadatas):
            batch.add_data_object(
                data_object=meta,
                class_name=class_name,
                vector=emb,
                uuid=uid[:36] if len(uid) >= 36 else None,
            )
    print(f"  Upserted {len(ids)} objects into Weaviate class '{class_name}'.")


BACKENDS = {
    "chroma":   ingest_chroma,
    "qdrant":   ingest_qdrant,
    "weaviate": ingest_weaviate,
}


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> int:
    parser = argparse.ArgumentParser(
        description="Ingest a ScanTailor RAG export into a vector store."
    )
    parser.add_argument("export_dir", help="Path to the ScanTailor RAG export directory")
    parser.add_argument(
        "--collection", default="scantailor_docs",
        help="Target collection / index name (default: scantailor_docs)"
    )
    parser.add_argument(
        "--backend", choices=list(BACKENDS), default="chroma",
        help="Vector store backend (default: chroma)"
    )
    parser.add_argument(
        "--embed", choices=list(EMBEDDERS), default="clip",
        help="Embedding strategy: 'clip' (CLIP model) or 'none' (zero vectors)"
    )
    parser.add_argument("--host", default="", help="Backend host (default: localhost)")
    parser.add_argument("--port", type=int, default=0, help="Backend port")
    args = parser.parse_args()

    export_dir = Path(args.export_dir)
    manifest_path = export_dir / "manifest.json"

    if not manifest_path.exists():
        print(f"Error: manifest.json not found in '{export_dir}'", file=sys.stderr)
        return 1

    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    pages = manifest.get("pages", [])

    if not pages:
        print("Warning: manifest contains no pages.", file=sys.stderr)
        return 0

    print(
        f"Project : {manifest.get('project_name', '?')}\n"
        f"Pages   : {manifest.get('page_count', len(pages))}\n"
        f"Backend : {args.backend}\n"
        f"Embed   : {args.embed}\n"
        f"Coll.   : {args.collection}\n"
    )

    image_paths = [export_dir / p["image_file"] for p in pages]
    missing = [str(p) for p in image_paths if not p.exists()]
    if missing:
        print(f"Warning: {len(missing)} image(s) not found:", file=sys.stderr)
        for m in missing[:5]:
            print(f"  {m}", file=sys.stderr)

    # Build IDs and metadata records
    ids = [str(p["page_id"]) for p in pages]
    metadatas: list[dict[str, Any]] = []
    for p in pages:
        meta: dict[str, Any] = {
            "index":           p.get("index", 0),
            "page_id":         str(p.get("page_id", "")),
            "original_source": str(p.get("original_source", "")),
            "image_file":      str(p.get("image_file", "")),
            "width_px":        int(p.get("width_px", 0)),
            "height_px":       int(p.get("height_px", 0)),
            "source_dpi_x":    int(p.get("source_dpi_x", 0)),
            "source_dpi_y":    int(p.get("source_dpi_y", 0)),
            "project_name":    str(manifest.get("project_name", "")),
        }
        metadatas.append(meta)

    # Compute embeddings
    print("Computing embeddings …")
    embeddings = EMBEDDERS[args.embed](image_paths)

    # Ingest
    print("Ingesting into vector store …")
    BACKENDS[args.backend](
        ids=ids,
        embeddings=embeddings,
        metadatas=metadatas,
        collection=args.collection,
        host=args.host,
        port=args.port,
    )

    print(f"\nDone. {len(ids)} page(s) ingested.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
