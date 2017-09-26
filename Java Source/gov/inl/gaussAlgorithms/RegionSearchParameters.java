/*
 * Copyright 2017 Battelle Energy Alliance
 */
/*
 *  Gauss Algorithms
 *
 *  File Name: RegionSearchParameters.java
 *
 *  Description: holds the region search parameters
 */
package gov.inl.gaussAlgorithms;

/**
 * miscellaneous settings used by a region search
 *
 */
public class RegionSearchParameters {
	
	/**
	 * use "FORPEAKS" mode to set up only regions containing peaks
	 *
	 */
	public static enum SEARCHMODE { ALL, FORPEAKS };

	// default values

	private final static SEARCHMODE DEFAULT_SEARCHMODE = SEARCHMODE.ALL;
	private final static double DEFAULT_THRESHOLD = 2;
	private final static int DEFAULT_MAXEXPANSIONCHANS = 3;
	private final static int DEFAULT_SUBTRENDSCHANS = 2;
	private final static int DEFAULT_MAXWIDTHCHANS = 150;
	private final static int DEFAULT_MAX_COUNT = 400;

	// member data

	/*
	 *			for low search sensitivity, use  threshold = 3
	 *			for med search sensitivity, use  threshold = 2
	 *			for high search sensitivity, use threshold = 1
	 *
	 *			'irw' and 'irch' control padding the ends of
	 *			the found regions.  The pad starts with
	 *			the gap between regions, is decremented by irch,
	 *			and is increased to 'irw * peakwidth'.
	 *
	 *			recommended starting value for  irw = 3
	 *			recommended starting value for irch = 2
	 *
	 *			maxrgnwid limits the size of each region (number
	 *			of channels in a region).  A good value to use
	 *			is maxrgnwid = 150.
	 */

	private SEARCHMODE             m_searchMode;
	private double                 m_threshold;
	private int                    m_maxExpansionChannels;
	private int                    m_subtractEndsChannels;
	private int                    m_maxWidthChannels;
	private int                    m_maxCount;

	// constructors

	public RegionSearchParameters() {
		
		setSearchMode(DEFAULT_SEARCHMODE);
		setThreshold(DEFAULT_THRESHOLD);
		setMaxExpansionChannels(DEFAULT_MAXEXPANSIONCHANS);
		setSubtractEndsChannels(DEFAULT_SUBTRENDSCHANS);
		setMaxWidthChannels(DEFAULT_MAXWIDTHCHANS);
		setMaxCount(DEFAULT_MAX_COUNT);
	}

	public RegionSearchParameters(SEARCHMODE searchMode,
			double threshold, int maxExpansionChannels,
			int subtractEndsChannels, int maxWidthChannels,
			int maxCount) {

		setSearchMode(searchMode);
		setThreshold(threshold);
		setMaxExpansionChannels(maxExpansionChannels);
		setSubtractEndsChannels(subtractEndsChannels);
		setMaxWidthChannels(maxWidthChannels);
		setMaxCount(maxCount);
	}

	// public methods
	
	public int getMaxCount() {
		
		return m_maxCount;
	}
	
	public int getMaxExpansionChannels() {
		
		return m_maxExpansionChannels;
	}
	
	public int getMaxWidthChannels() {
		
		return m_maxWidthChannels;
	}
	
	public SEARCHMODE getSearchMode() {

		return m_searchMode;
	}
	
	public int getSubtractEndsChannels() {
		
		return m_subtractEndsChannels;
	}
	
	public double getThreshold() {
		
		return m_threshold;
	}

	public void setMaxCount(int maxCount) {
		
		m_maxCount = maxCount;
	}
	
	public void setMaxExpansionChannels(int maxExpansionChannels) {
		
		m_maxExpansionChannels = maxExpansionChannels;
	}
	
	public void setMaxWidthChannels(int maxWidthChannels) {
		
		m_maxWidthChannels = maxWidthChannels;
	}
	
	public void setSearchMode(SEARCHMODE searchMode) {
		
		m_searchMode = searchMode;
	}
	
	public void setSubtractEndsChannels(int subtractEndsChannels) {
		
		m_subtractEndsChannels = subtractEndsChannels;
	}
	
	public void setThreshold(double threshold) {
		
		m_threshold = threshold;
	}
}
