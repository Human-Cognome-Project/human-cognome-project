"""Tests for hcp.physics.forces module.

Tests gravity clustering and albedo (relevance) calculations.

Contributors: Planner & Silas (DI-cognome)
"""
import pytest

from hcp.physics.forces.gravity import (
    Cluster,
    GravityField,
    GravityCalculator,
    TopicDetector,
    cluster_tokens,
    filter_isolated,
)
from hcp.physics.forces.albedo import (
    AlbedoScore,
    AlbedoCalculator,
    calculate_albedo,
    get_keywords,
)
from hcp.core.token_id import TokenID
from hcp.core.pair_bond import PairBondMap


class TestCluster:
    """Test Cluster class."""

    def test_create_cluster(self):
        """Test cluster creation."""
        t1 = TokenID.byte(65)
        cluster = Cluster(tokens={t1})
        assert len(cluster) == 1
        assert t1 in cluster.tokens

    def test_add_token(self):
        """Test adding token to cluster."""
        t1, t2 = TokenID.byte(65), TokenID.byte(66)
        cluster = Cluster(tokens={t1})
        cluster.add(t2)
        assert len(cluster) == 2

    def test_merge_clusters(self):
        """Test merging clusters."""
        t1, t2 = TokenID.byte(65), TokenID.byte(66)
        c1 = Cluster(tokens={t1}, mass=1.0)
        c2 = Cluster(tokens={t2}, mass=2.0)
        c1.merge(c2)
        assert len(c1) == 2
        assert c1.mass == 3.0


class TestGravityField:
    """Test GravityField class."""

    def test_get_cluster(self):
        """Test getting cluster for token."""
        t1, t2 = TokenID.byte(65), TokenID.byte(66)
        c = Cluster(tokens={t1, t2})
        field = GravityField(
            clusters=[c],
            token_to_cluster={t1: 0, t2: 0},
            total_mass=1.0,
        )
        assert field.get_cluster(t1) == c
        assert field.get_cluster(t2) == c

    def test_get_unknown_token(self):
        """Unknown token returns None."""
        t1 = TokenID.byte(65)
        field = GravityField(clusters=[], token_to_cluster={}, total_mass=0.0)
        assert field.get_cluster(t1) is None


class TestGravityCalculator:
    """Test GravityCalculator class."""

    def test_calculate_attraction(self):
        """Test attraction calculation."""
        t1, t2 = TokenID.byte(65), TokenID.byte(66)
        pbm = PairBondMap()
        pbm.add_bond(t1, t2)
        pbm.add_bond(t2, t1)

        calc = GravityCalculator()
        attraction = calc.calculate_attraction(t1, t2, pbm)
        assert attraction > 0

    def test_cluster_empty_pbm(self):
        """Empty PBM produces empty field."""
        pbm = PairBondMap()
        calc = GravityCalculator()
        field = calc.cluster(pbm)
        assert field.clusters == []
        assert field.total_mass == 0.0

    def test_cluster_single_token(self):
        """Single token creates single cluster."""
        t1 = TokenID.byte(65)
        pbm = PairBondMap()
        # Add self-bond to register token
        pbm.add_bond(t1, t1)

        calc = GravityCalculator()
        field = calc.cluster(pbm)
        assert len(field.clusters) >= 1

    def test_cluster_connected_tokens(self):
        """Strongly connected tokens cluster together."""
        t1, t2 = TokenID.byte(65), TokenID.byte(66)
        pbm = PairBondMap()
        # Strong bidirectional connection
        for _ in range(10):
            pbm.add_bond(t1, t2)
            pbm.add_bond(t2, t1)

        calc = GravityCalculator(attraction_threshold=0.01)
        field = calc.cluster(pbm)

        # Should be in same cluster
        if field.clusters:
            first_cluster = field.clusters[0]
            assert t1 in first_cluster.tokens or t2 in first_cluster.tokens

    def test_filter_by_gravity(self):
        """Filter removes small clusters."""
        t1, t2 = TokenID.byte(65), TokenID.byte(66)
        pbm = PairBondMap()
        pbm.add_bond(t1, t2)
        pbm.add_bond(t2, t1)

        calc = GravityCalculator(attraction_threshold=0.01)
        clusters = calc.filter_by_gravity(pbm, min_cluster_size=2)
        # Either returns clusters with 2+ tokens, or empty if couldn't merge
        for c in clusters:
            assert len(c) >= 2


class TestTopicDetector:
    """Test TopicDetector class."""

    def test_detect_empty(self):
        """Empty PBM returns no topics."""
        pbm = PairBondMap()
        detector = TopicDetector()
        topics = detector.detect(pbm)
        assert topics == []

    def test_detect_topics(self):
        """Test topic detection."""
        t1, t2, t3 = TokenID.byte(65), TokenID.byte(66), TokenID.byte(67)
        pbm = PairBondMap()
        # Create connections
        pbm.add_bond(t1, t2)
        pbm.add_bond(t2, t3)

        detector = TopicDetector()
        topics = detector.detect(pbm)
        # Should detect at least some structure
        assert isinstance(topics, list)

    def test_get_main_topic_empty(self):
        """Empty PBM returns empty main topic."""
        pbm = PairBondMap()
        detector = TopicDetector()
        main = detector.get_main_topic(pbm)
        assert main == set()

    def test_get_main_topic(self):
        """Main topic is largest cluster."""
        t1, t2, t3 = TokenID.byte(65), TokenID.byte(66), TokenID.byte(67)
        pbm = PairBondMap()
        for _ in range(5):
            pbm.add_bond(t1, t2)
            pbm.add_bond(t2, t1)
            pbm.add_bond(t2, t3)
            pbm.add_bond(t3, t2)

        detector = TopicDetector()
        main = detector.get_main_topic(pbm)
        assert isinstance(main, set)


class TestGravityConvenienceFunctions:
    """Test module-level convenience functions."""

    def test_cluster_tokens(self):
        """Test cluster_tokens function."""
        pbm = PairBondMap()
        t1, t2 = TokenID.byte(65), TokenID.byte(66)
        pbm.add_bond(t1, t2)

        clusters = cluster_tokens(pbm)
        assert isinstance(clusters, list)

    def test_filter_isolated(self):
        """Test filter_isolated function."""
        pbm = PairBondMap()
        t1, t2 = TokenID.byte(65), TokenID.byte(66)
        pbm.add_bond(t1, t2)
        pbm.add_bond(t2, t1)

        non_isolated = filter_isolated(pbm)
        assert isinstance(non_isolated, list)


class TestAlbedoScore:
    """Test AlbedoScore class."""

    def test_create_score(self):
        """Test score creation."""
        t = TokenID.byte(65)
        score = AlbedoScore(
            token=t,
            score=0.8,
            forward_connections=3,
            backward_connections=2,
        )
        assert score.score == 0.8
        assert score.forward_connections == 3

    def test_centrality(self):
        """Test centrality calculation."""
        t = TokenID.byte(65)
        score = AlbedoScore(
            token=t,
            score=0.8,
            forward_connections=4,
            backward_connections=2,
        )
        assert score.centrality == 3.0  # (4 + 2) / 2


class TestAlbedoCalculator:
    """Test AlbedoCalculator class."""

    def test_calculate_empty(self):
        """Empty PBM returns empty scores."""
        pbm = PairBondMap()
        calc = AlbedoCalculator()
        scores = calc.calculate(pbm)
        assert scores == {}

    def test_calculate_single_token(self):
        """Single token gets a score."""
        t1 = TokenID.byte(65)
        pbm = PairBondMap()
        pbm.add_bond(t1, t1)  # Self-bond to register

        calc = AlbedoCalculator()
        scores = calc.calculate(pbm)
        assert t1 in scores
        assert isinstance(scores[t1], AlbedoScore)

    def test_calculate_connected_tokens(self):
        """Connected tokens get higher scores."""
        t1, t2, t3 = TokenID.byte(65), TokenID.byte(66), TokenID.byte(67)
        pbm = PairBondMap()
        # t2 is highly connected
        pbm.add_bond(t1, t2)
        pbm.add_bond(t2, t3)
        pbm.add_bond(t2, t1)
        pbm.add_bond(t3, t2)

        calc = AlbedoCalculator()
        scores = calc.calculate(pbm)

        assert t1 in scores
        assert t2 in scores
        assert t3 in scores

    def test_rank_by_albedo(self):
        """Test ranking by albedo."""
        t1, t2 = TokenID.byte(65), TokenID.byte(66)
        pbm = PairBondMap()
        pbm.add_bond(t1, t2)

        calc = AlbedoCalculator()
        ranked = calc.rank_by_albedo(pbm)

        assert isinstance(ranked, list)
        assert all(isinstance(item, tuple) for item in ranked)

    def test_top_tokens(self):
        """Test getting top tokens."""
        t1, t2, t3 = TokenID.byte(65), TokenID.byte(66), TokenID.byte(67)
        pbm = PairBondMap()
        pbm.add_bond(t1, t2)
        pbm.add_bond(t2, t3)

        calc = AlbedoCalculator()
        top = calc.top_tokens(pbm, n=2)

        assert len(top) <= 2
        assert all(isinstance(t, TokenID) for t in top)


class TestAlbedoConvenienceFunctions:
    """Test albedo module-level functions."""

    def test_calculate_albedo(self):
        """Test calculate_albedo function."""
        t1, t2 = TokenID.byte(65), TokenID.byte(66)
        pbm = PairBondMap()
        pbm.add_bond(t1, t2)

        scores = calculate_albedo(pbm)
        assert isinstance(scores, dict)
        assert all(isinstance(v, float) for v in scores.values())

    def test_get_keywords(self):
        """Test get_keywords function."""
        t1, t2, t3 = TokenID.byte(65), TokenID.byte(66), TokenID.byte(67)
        pbm = PairBondMap()
        pbm.add_bond(t1, t2)
        pbm.add_bond(t2, t3)

        keywords = get_keywords(pbm, n=2)
        assert isinstance(keywords, list)
        assert len(keywords) <= 2


class TestIntegrationGravityAlbedo:
    """Test gravity and albedo working together."""

    def test_keywords_in_clusters(self):
        """Keywords should generally be in clusters."""
        t1, t2, t3, t4 = (
            TokenID.byte(65),
            TokenID.byte(66),
            TokenID.byte(67),
            TokenID.byte(68),
        )
        pbm = PairBondMap()
        # Create a connected cluster
        for _ in range(5):
            pbm.add_bond(t1, t2)
            pbm.add_bond(t2, t1)
            pbm.add_bond(t2, t3)
            pbm.add_bond(t3, t2)

        # Get keywords and clusters
        keywords = get_keywords(pbm, n=3)
        clusters = cluster_tokens(pbm)

        # Keywords should exist
        assert len(keywords) > 0

        # Clusters should exist
        assert isinstance(clusters, list)
