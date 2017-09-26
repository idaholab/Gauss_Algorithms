/*
 * Copyright 2017 Battelle Energy Alliance
 */
/*
 *  Gauss Algorithms
 *
 *  File Name: WidthEquation.java
 *
 *  Description: Implements the width equation
 */
package gov.inl.gaussAlgorithms;

/**
 * the width equation contains the peak width calibration
 *
 */public class WidthEquation implements Cloneable {

		/**
		 * this indicates the width equation form:
		 * w(x) = alpha + beta*x
		 * or
		 * w(x) = sqrt(alpha + beta*x)
		 *
		 */
	public static enum MODE {
		
		LINEAR("linear"),
		SQUARE_ROOT("square root");
	
		// member data
		private final String m_label;
	
		MODE(String label) {
			m_label = label;
		}
		public String label() {
			return m_label;
		}
	};

	/*
	 * GLWidthEqn holds coefficients for the width equation
	 *
	 * linear: w(x) = alpha + beta*x
	 *
	 * sqrt:   w(x) = sqrt(alpha + beta*x)
	 *
	 * and chi squared from corresponding calibration.
	 */
	private final double m_alpha;
	private final double m_beta;
	private final double m_chi_sq;
	private final MODE m_mode;

	public WidthEquation(double constCoeff, double linCoeff, double chi_sq,
						 MODE mode) {

		m_alpha = constCoeff;
		m_beta = linCoeff;
		m_chi_sq = chi_sq;
		m_mode = mode;
	}

	public WidthEquation clone() {
		
		WidthEquation eqn = new WidthEquation(m_alpha, m_beta, m_chi_sq,
				m_mode);

		return eqn;
	}
	
	public boolean equals(Object other) {
		
		if (null == other) {
			return false;
		}
		
		if (!(other instanceof WidthEquation)) {
			return false;
		}
		
		boolean answer = false;
		
		WidthEquation owx = (WidthEquation) other;
		
		if ((m_alpha == owx.m_alpha) && (m_beta == owx.m_beta) &&
			(m_mode == owx.m_mode)) {
			answer = true;
		}

		return answer;
	}

	public double getChiSq() {
		return m_chi_sq;
	}

	public double getConstantCoefficient() {
		return m_alpha;
	}

	public double getLinearCoefficient() {
		return m_beta;
	}
	
	public MODE getMode() {
		return m_mode;
	}

	public double getPeakwidth(double channel) throws Exception {
		
		double width = m_alpha + (m_beta * channel);
		
		if (width < 0) {
			throw new Exception("peakwidth negative or undefined");
		}
		
		if (WidthEquation.MODE.SQUARE_ROOT.equals(m_mode)) {
			width = Math.sqrt(width);
		}
		
		return width;
	}
	
	public int hashCode() {
		
		return toString().hashCode();
	}
	
	public String toDisplayString() {
		
		String betaSign = " ";
		
		if (0 <= m_beta) {
			betaSign = " +";
		}
		
		if (MODE.LINEAR.equals(m_mode)) {
			return "w(x) = " + m_alpha + betaSign + m_beta + "X";
		} else {
			return "w(x) = sqrt(" + m_alpha + betaSign + m_beta + "X)";
		}
	}

	public String toString() {
		
		String answer = null;
		if (MODE.LINEAR.equals(m_mode)) {
			answer = "w(x) = " + m_alpha + " + " + m_beta + "X (chisq = " +
				m_chi_sq + ")";
		} else {
			answer = "w(x) = sqrt(" + m_alpha + " + " + m_beta +
				"X)   (chisq = " + m_chi_sq + ")";
		}

		return answer;
	}
}
