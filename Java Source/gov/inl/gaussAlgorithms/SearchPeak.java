/*
 * Copyright 2017 Battelle Energy Alliance
 */
/*
 *  Gauss Algorithms
 *
 *  File Name: SearchPeak.java
 *
 *  Description: holds centroid and ratio as returned from a peak search
 */
package gov.inl.gaussAlgorithms;

/**
 * describes a peak returned by a peak search
 *
 */
public class SearchPeak implements Comparable<Object> {
	
	protected static final float THRESHOLD = (float) .00001;

	// member data
	
	private final int             m_rawCentroid;
	private final ChannelRange    m_refineRegion;
	private final double          m_netArea;
	private final double          m_background;
	private final double          m_refinedCentroid;
	private final double          m_ratio;     // of area to sigma
	private final boolean         m_useRefinement;
	
	// constructor
	
	public SearchPeak(int rawCentroid) {
		
		this(rawCentroid, null, 0, 0, rawCentroid, false);
	}
	
	public SearchPeak(int rawCentroid, ChannelRange refineRegion,
			double netArea, double background, double refinedCentroid,
			boolean useRefinement) {
		
		m_rawCentroid = rawCentroid;
		m_refineRegion = refineRegion;
		m_netArea = netArea;
		m_background = background;
		m_refinedCentroid = refinedCentroid;
		m_useRefinement = useRefinement;
		
		double areaUncertainty = Math.sqrt(Math.abs(netArea) +
											(2 * Math.abs(background)));
		
		if (areaUncertainty < THRESHOLD) {
			areaUncertainty = 1;
		}
		m_ratio = netArea / areaUncertainty;
	}
	
	// public methods

	public int compareTo(Object otherObject) {
		
		if (!(otherObject instanceof SearchPeak)) {
			return -1;
		}
		
		SearchPeak otherPeak = (SearchPeak) otherObject;
		
		int answer = compareDouble(this.getUseCentroid(),
				otherPeak.getUseCentroid());
		if (0 == answer) {
			answer = compareDouble(this.m_netArea, otherPeak.m_netArea);
		}
		
		return answer;
	}
	
	public double getArea() {
		return m_netArea;
	}
	
	public double getBackground() {
		return m_background;
	}
	
	public double getRatio() {
		return m_ratio;
	}
	
	public int getRawCentroid() {
		return m_rawCentroid;
	}
	
	public double getRefinedCentroid() {
		return m_refinedCentroid;
	}
	
	public ChannelRange getRefineRegion() {
		return m_refineRegion;
	}
	
	public double getUseCentroid() {
		if (m_useRefinement) {
			return m_refinedCentroid;
		} else {
			return m_rawCentroid;
		}
	}
	
	public int hashCode() {
		
		int code = (int) (getUseCentroid() * 1000);
		code = code + (int) (m_netArea * 100);

		return code;
	}
	
	//
	// If the search decides that the refinement is best, then
	// this flag is set to true.
	//
	public boolean useRefinement() {
		return m_useRefinement;
	}

	// private methods

	private int compareDouble(double fromThisPair, double fromOtherPair) {

		int answer = 0;

		double diff = Math.abs(fromThisPair - fromOtherPair);
		if (diff > THRESHOLD) {
			answer = Double.compare(fromThisPair, fromOtherPair);
		}

		return answer;
	}
}
