#!/usr/bin/env python3
"""
Test the new neutral LLM engine with evidence-based analysis
"""

from vulnrag_retriever import quick_vulnrag_analysis
from llm_engine import quick_vulnerability_analysis, validate_analysis_consistency

# Test code examples
vuln_code = '''
static void ttm_put_pages(struct page **pages, unsigned npages, int flags,
			  enum ttm_caching_state cstate)
{
	struct ttm_page_pool *pool = ttm_get_pool(flags, false, cstate);
	unsigned long irq_flags;
	unsigned i;

	if (pool == NULL) {
		i = 0;
		while (i < npages) {
			unsigned order = 0, j;

			if (!pages[i]) {
				++i;
				continue;
			}

			if (page_count(pages[i]) != 1)
				pr_err("Erroneous page count. Leaking pages.\\n");
			__free_pages(pages[i], order);

			j = 1 << order;
			while (j) {
				pages[i++] = NULL;
				--j;
			}
		}
		return;
	}
}
'''

safe_code = '''
static int kvm_vm_ioctl_create_vcpu(struct kvm *kvm, u32 id)
{
	int r;
	struct kvm_vcpu *vcpu, *v;

	if (id >= KVM_MAX_VCPUS)
		return -EINVAL;

	vcpu = kvm_arch_vcpu_create(kvm, id);
	if (IS_ERR(vcpu))
		return PTR_ERR(vcpu);

	mutex_lock(&kvm->lock);
	if (!kvm_vcpu_compatible(vcpu)) {
		r = -EINVAL;
		goto unlock_vcpu_destroy;
	}
	if (atomic_read(&kvm->online_vcpus) == KVM_MAX_VCPUS) {
		r = -EINVAL;
		goto unlock_vcpu_destroy;
	}

	kvm_for_each_vcpu(r, v, kvm)
		if (v->vcpu_id == id) {
			r = -EEXIST;
			goto unlock_vcpu_destroy;
		}

	mutex_unlock(&kvm->lock);
	return r;

unlock_vcpu_destroy:
	mutex_unlock(&kvm->lock);
	return r;
}
'''

def test_vulnerable_code():
    """Test vulnerable code analysis with neutral approach"""
    print("🔍 Testing VULNERABLE Code Analysis (Neutral)...")
    print("=" * 70)
    
    # Get retrieval results with evidence
    retrieval_results = quick_vulnrag_analysis(vuln_code, top_k=3)
    print(f"📊 Retrieval Classification: {retrieval_results.classification}")
    print(f"📊 Best VULN Score: {retrieval_results.best_vuln_score:.3f}")
    print(f"📊 Best PATCH Score: {retrieval_results.best_patch_score:.3f}")
    print(f"📊 Evidence FOR: {len(retrieval_results.evidence_for)} patterns")
    print(f"📊 Evidence AGAINST: {len(retrieval_results.evidence_against)} patterns")
    
    # Get LLM analysis
    llm_analysis = quick_vulnerability_analysis(vuln_code, retrieval_results)
    
    print(f"\n🤖 Neutral LLM Analysis:")
    print(f"📊 Is Vulnerable: {llm_analysis.is_vulnerable}")
    print(f"📊 Confidence: {llm_analysis.confidence_score:.3f}")
    print(f"📊 Vulnerability Type: {llm_analysis.vulnerability_type}")
    print(f"📊 CWE Prediction: {llm_analysis.cwe_prediction}")
    print(f"📊 Risk Level: {llm_analysis.risk_level}")
    print(f"📊 Classification: {llm_analysis.classification}")
    print(f"📝 Explanation: {llm_analysis.explanation[:200]}...")
    
    # Show evidence
    print(f"\n📋 Evidence FOR ({len(llm_analysis.evidence_for)}):")
    for i, evidence in enumerate(llm_analysis.evidence_for[:2]):
        print(f"  {i+1}. {evidence}")
    
    print(f"\n📋 Evidence AGAINST ({len(llm_analysis.evidence_against)}):")
    for i, evidence in enumerate(llm_analysis.evidence_against[:2]):
        print(f"  {i+1}. {evidence}")
    
    print(f"\n⚠️ Limitations ({len(llm_analysis.limitations)}):")
    for i, limitation in enumerate(llm_analysis.limitations[:2]):
        print(f"  {i+1}. {limitation}")
    
    # Validate consistency
    validation = validate_analysis_consistency(llm_analysis)
    print(f"\n🔍 Analysis Validation:")
    print(f"  Consistent: {validation['is_consistent']}")
    print(f"  Evidence Balance: {validation['evidence_balance']:.3f}")
    if validation['issues']:
        print(f"  Issues: {validation['issues']}")

def test_safe_code():
    """Test safe code analysis with neutral approach"""
    print("\n🔍 Testing SAFE Code Analysis (Neutral)...")
    print("=" * 70)
    
    # Get retrieval results with evidence
    retrieval_results = quick_vulnrag_analysis(safe_code, top_k=3)
    print(f"📊 Retrieval Classification: {retrieval_results.classification}")
    print(f"📊 Best VULN Score: {retrieval_results.best_vuln_score:.3f}")
    print(f"📊 Best PATCH Score: {retrieval_results.best_patch_score:.3f}")
    print(f"📊 Evidence FOR: {len(retrieval_results.evidence_for)} patterns")
    print(f"📊 Evidence AGAINST: {len(retrieval_results.evidence_against)} patterns")
    
    # Get LLM analysis
    llm_analysis = quick_vulnerability_analysis(safe_code, retrieval_results)
    
    print(f"\n🤖 Neutral LLM Analysis:")
    print(f"📊 Is Vulnerable: {llm_analysis.is_vulnerable}")
    print(f"📊 Confidence: {llm_analysis.confidence_score:.3f}")
    print(f"📊 Vulnerability Type: {llm_analysis.vulnerability_type}")
    print(f"📊 CWE Prediction: {llm_analysis.cwe_prediction}")
    print(f"📊 Risk Level: {llm_analysis.risk_level}")
    print(f"📊 Classification: {llm_analysis.classification}")
    print(f"📝 Explanation: {llm_analysis.explanation[:200]}...")
    
    # Show evidence
    print(f"\n📋 Evidence FOR ({len(llm_analysis.evidence_for)}):")
    for i, evidence in enumerate(llm_analysis.evidence_for[:2]):
        print(f"  {i+1}. {evidence}")
    
    print(f"\n📋 Evidence AGAINST ({len(llm_analysis.evidence_against)}):")
    for i, evidence in enumerate(llm_analysis.evidence_against[:2]):
        print(f"  {i+1}. {evidence}")
    
    print(f"\n⚠️ Limitations ({len(llm_analysis.limitations)}):")
    for i, limitation in enumerate(llm_analysis.limitations[:2]):
        print(f"  {i+1}. {limitation}")
    
    # Validate consistency
    validation = validate_analysis_consistency(llm_analysis)
    print(f"\n🔍 Analysis Validation:")
    print(f"  Consistent: {validation['is_consistent']}")
    print(f"  Evidence Balance: {validation['evidence_balance']:.3f}")
    if validation['issues']:
        print(f"  Issues: {validation['issues']}")

if __name__ == "__main__":
    test_vulnerable_code()
    test_safe_code() 