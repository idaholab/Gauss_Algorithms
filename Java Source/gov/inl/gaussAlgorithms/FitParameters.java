/*
 * Copyright 2017 Battelle Energy Alliance
 */
/*
 *  Gauss Algorithms
 *
 *  File Name: FitParameters.java
 *
 *  Description: holds fit parameters
 */
package gov.inl.gaussAlgorithms;

/**
 * holds miscellaneous settings used to fit a region
 *
 */
public class FitParameters implements Cloneable {

	/*
	 * enumerations of legal fit parameter values
	 */

	/**
	 * enumeration of peak width modes which are used to set the
	 * convergence criteria and determine when to vary peak width.
	 * <p>
	 * See the description of CCType for a discussion of setting
	 * the convergence criteria.
	 * <p>
	 * For determining at beginning of each fit cycle whether
	 * to vary peak width:
	 * <p>
	 * <pre>
	 * if mode == PEAKWIDTH_VARIES:
	 *    if at least one peak centroid is not fixed,
	 *       let average peak width vary.
	 *    if jth peak is ~511keV and it is not only peak,
	 *       let fitinfo.peakinfo[j].addwidth_511 vary.
	 * 
	 * if mode == PEAKWIDTH_FIXED:
	 *    if jth peak is ~511keV,
	 *       let fitinfo.peakinfo[j].addwidth_511 vary.
	 * </pre>
	 * 
	 */
	public static enum PeakWidthMode { VARIES, FIXED };

	/**
	 * enumeration of convergence criteria types.
	 * <p>
	 * CCType is used in conjunction with PeakWidthMode to choose values
	 * for the convergence criteria ftol and xtol, which are inputs to
	 * the Levenberg-Marquardt algorithm lmder().
	 * <p>
	 * ftol measures the relative error desired in the sum of squares.
	 * xtol measures the relative error desired in the approximate solution.
	 * <p>
	 * setting ftol and xtol:
	 * <p>
	 * <pre>
	 * if type == CC_LARGER and mode == PEAKWIDTH_VARIES
	 *    then ftol = .001
	 *    and  xtol = .003
	 * 
	 * if type == CC_LARGER and mode == PEAKWIDTH_FIXED
	 *    then ftol = xtol = .01
	 * 
	 * if type == CC_SMALLER
	 *    then ftol = xtol = .001
	 * 
	 * if type == CC_LARGER_INC
	 *    then ftol = .001
	 *    and  xtol = .003
	 * </pre>
	 * 
	 */
	public static enum CCType { 
		LARGER("Large"), 
		SMALLER("Small"), 
		LARGER_INC("Large(increase allowed)");
		
		// member data
		private final String m_label;
		// constructor
		private CCType(String label) {
			m_label = label;
		}
		// public method
		public String displayLabel() {
			return m_label;
		}
	};
	
	// default values

	private int               DEFAULT_NCYCLE = 10;
    private int               DEFAULT_NOUT = 1;
	private int               DEFAULT_MAXNPEAKS = 10;
	private PeakWidthMode     DEFAULT_PKWDMODE = PeakWidthMode.VARIES;
	private CCType            DEFAULT_CCTYPE = CCType.LARGER;
	private float             DEFAULT_RESIDUAL = (float) 20.0;

	// member data

	private int                    m_nCycle;
	private int                    m_nOut;
	private int                    m_maxNpeaks;
	private float                  m_maxResid;
	private PeakWidthMode          m_peakwidthMode;
	private CCType                 m_ccType;

	// constructors

	public FitParameters() {

		setNcycle(DEFAULT_NCYCLE);
		setNout(DEFAULT_NOUT);
		setMaxNpeaks(DEFAULT_MAXNPEAKS);
		setPeakwidthMode(DEFAULT_PKWDMODE);
		setCcType(DEFAULT_CCTYPE);
		setMaxResid(DEFAULT_RESIDUAL);
	}

	public FitParameters(int nCycle, int nOut, int maxNpeaks,
						 PeakWidthMode peakwidthMode, CCType ccType,
						 float maxResid) {

		setNcycle(nCycle);
		setNout(nOut);
		setMaxNpeaks(maxNpeaks);
		setPeakwidthMode(peakwidthMode);
		setCcType(ccType);
		setMaxResid(maxResid);
	}

	// public methods
	
	public Object clone() {
		
		return new FitParameters(m_nCycle, m_nOut, m_maxNpeaks,
				m_peakwidthMode, m_ccType, m_maxResid);
	}

	public CCType getCcType() {

		return m_ccType;
	}

	public int getMaxNpeaks() {

		return m_maxNpeaks;
	}

	public float getMaxResid() {

		return m_maxResid;
	}

	public int getNcycle() {

		return m_nCycle;
	}

	public int getNout() {

		return m_nOut;
	}

	public PeakWidthMode getPeakwidthMode() {

		return m_peakwidthMode;
	}

	public void setCcType(CCType ccType) {

		m_ccType = ccType;
	}

	public void setMaxNpeaks(int maxNpeaks) {

		m_maxNpeaks = maxNpeaks;
	}

	public void setMaxResid(float maxResid) {

		m_maxResid = maxResid;
	}

	public void setNcycle(int nCycle) {

		m_nCycle = nCycle;
	}

	public void setNout(int nOut) {

		m_nOut = nOut;
	}

	public void setPeakwidthMode(PeakWidthMode peakwidthMode) {

		m_peakwidthMode = peakwidthMode;
	}
}
