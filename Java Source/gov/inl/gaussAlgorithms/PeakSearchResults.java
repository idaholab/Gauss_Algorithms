/*
 * Copyright 2017 Battelle Energy Alliance
 */
/*
 *  Gauss Algorithms
 *
 *  File Name: PeakSearchResults.java
 *
 *  Description: holds the answer returned from a peak search
 */
package gov.inl.gaussAlgorithms;

import java.util.Collection;
import java.util.HashMap;
import java.util.TreeSet;

/**
 * returned by a peak search
 *
 */
public class PeakSearchResults {
	
	// member data
	
	// maps the "raw" centroid to the search peak information
	private final HashMap<Integer, SearchPeak>      m_peakRefinements;
	private int[]                                   m_crossProducts;
	
	// constructor
	
	public PeakSearchResults() {
		
		m_peakRefinements = new HashMap<Integer, SearchPeak>();
		m_crossProducts = new int[0];
	}
	
	// public methods
	
	public void addPeak(SearchPeak refinement) {
		
		Integer key = new Integer(refinement.getRawCentroid());
		m_peakRefinements.put(key, refinement);
	}
	
	public int[] getCrossProducts() {
		
		return m_crossProducts;
	}	
	
	public TreeSet<SearchPeak> getSearchPeakList() {
		
		Collection<SearchPeak> values = m_peakRefinements.values();
		TreeSet<SearchPeak> peakList = new TreeSet<SearchPeak>();
		peakList.addAll(values);
		return peakList;
	}
	
	public void setCrossProducts(int[] crossProducts) {
		
		m_crossProducts = crossProducts;
	}
}
