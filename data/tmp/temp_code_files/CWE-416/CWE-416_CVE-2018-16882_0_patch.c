static void nested_get_vmcs12_pages(struct kvm_vcpu *vcpu)
{
	struct vmcs12 *vmcs12 = get_vmcs12(vcpu);
	struct vcpu_vmx *vmx = to_vmx(vcpu);
	struct page *page;
	u64 hpa;

	if (nested_cpu_has2(vmcs12, SECONDARY_EXEC_VIRTUALIZE_APIC_ACCESSES)) {
		/*
		 * Translate L1 physical address to host physical
		 * address for vmcs02. Keep the page pinned, so this
		 * physical address remains valid. We keep a reference
		 * to it so we can release it later.
		 */
		if (vmx->nested.apic_access_page) { /* shouldn't happen */
			kvm_release_page_dirty(vmx->nested.apic_access_page);
			vmx->nested.apic_access_page = NULL;
		}
		page = kvm_vcpu_gpa_to_page(vcpu, vmcs12->apic_access_addr);
		/*
		 * If translation failed, no matter: This feature asks
		 * to exit when accessing the given address, and if it
		 * can never be accessed, this feature won't do
		 * anything anyway.
		 */
		if (!is_error_page(page)) {
			vmx->nested.apic_access_page = page;
			hpa = page_to_phys(vmx->nested.apic_access_page);
			vmcs_write64(APIC_ACCESS_ADDR, hpa);
		} else {
			vmcs_clear_bits(SECONDARY_VM_EXEC_CONTROL,
					SECONDARY_EXEC_VIRTUALIZE_APIC_ACCESSES);
		}
	}

	if (nested_cpu_has(vmcs12, CPU_BASED_TPR_SHADOW)) {
		if (vmx->nested.virtual_apic_page) { /* shouldn't happen */
			kvm_release_page_dirty(vmx->nested.virtual_apic_page);
			vmx->nested.virtual_apic_page = NULL;
		}
		page = kvm_vcpu_gpa_to_page(vcpu, vmcs12->virtual_apic_page_addr);

		/*
		 * If translation failed, VM entry will fail because
		 * prepare_vmcs02 set VIRTUAL_APIC_PAGE_ADDR to -1ull.
		 * Failing the vm entry is _not_ what the processor
		 * does but it's basically the only possibility we
		 * have.  We could still enter the guest if CR8 load
		 * exits are enabled, CR8 store exits are enabled, and
		 * virtualize APIC access is disabled; in this case
		 * the processor would never use the TPR shadow and we
		 * could simply clear the bit from the execution
		 * control.  But such a configuration is useless, so
		 * let's keep the code simple.
		 */
		if (!is_error_page(page)) {
			vmx->nested.virtual_apic_page = page;
			hpa = page_to_phys(vmx->nested.virtual_apic_page);
			vmcs_write64(VIRTUAL_APIC_PAGE_ADDR, hpa);
		}
	}

	if (nested_cpu_has_posted_intr(vmcs12)) {
		if (vmx->nested.pi_desc_page) { /* shouldn't happen */
			kunmap(vmx->nested.pi_desc_page);
			kvm_release_page_dirty(vmx->nested.pi_desc_page);
			vmx->nested.pi_desc_page = NULL;
			vmx->nested.pi_desc = NULL;
			vmcs_write64(POSTED_INTR_DESC_ADDR, -1ull);
		}
		page = kvm_vcpu_gpa_to_page(vcpu, vmcs12->posted_intr_desc_addr);
		if (is_error_page(page))
			return;
		vmx->nested.pi_desc_page = page;
		vmx->nested.pi_desc = kmap(vmx->nested.pi_desc_page);
		vmx->nested.pi_desc =
			(struct pi_desc *)((void *)vmx->nested.pi_desc +
			(unsigned long)(vmcs12->posted_intr_desc_addr &
			(PAGE_SIZE - 1)));
		vmcs_write64(POSTED_INTR_DESC_ADDR,
			page_to_phys(vmx->nested.pi_desc_page) +
			(unsigned long)(vmcs12->posted_intr_desc_addr &
			(PAGE_SIZE - 1)));
	}
	if (nested_vmx_prepare_msr_bitmap(vcpu, vmcs12))
		vmcs_set_bits(CPU_BASED_VM_EXEC_CONTROL,
			      CPU_BASED_USE_MSR_BITMAPS);
	else
		vmcs_clear_bits(CPU_BASED_VM_EXEC_CONTROL,
				CPU_BASED_USE_MSR_BITMAPS);
}