/*
 * Copyright (c) 2020 MariaDB Corporation Ab
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file and at www.mariadb.com/bsl11.
 *
 * Change Date: 2026-03-29
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2 or later of the General
 * Public License.
 */

import mount from '@tests/unit/setup'
import PageWrapper from '@/components/common/PageWrapper'

describe('PageWrapper.vue', () => {
    let wrapper

    beforeEach(() => {
        wrapper = mount({
            shallow: false,
            component: PageWrapper,
        })
    })

    it(`Should render accurate content when default slot is used`, () => {
        wrapper = mount({
            shallow: true,
            component: PageWrapper,
            slots: {
                default: '<div class="dashboard-page">dashboard-page</div>',
            },
        })
        expect(wrapper.find('.dashboard-page').text()).to.be.equal('dashboard-page')
    })

    it(`Should not add 'wrapper-container' class when fluid props is enabled`, async () => {
        wrapper = mount({
            shallow: true,
            component: PageWrapper,
        })
        expect(wrapper.find('.wrapper-container').exists()).to.be.true
        await wrapper.setProps({ fluid: true })
        expect(wrapper.find('.wrapper-container').exists()).to.be.false
    })

    it(`Should add style to v-spacer when spacerStyle props has value`, () => {
        wrapper = mount({
            shallow: true,
            component: PageWrapper,
            propsData: {
                spacerStyle: { borderBottom: 'thin solid #e7eef1' },
            },
        })
        const spacer = wrapper.findComponent({ name: 'v-spacer' })
        expect(spacer.attributes().style).to.be.eql('border-bottom: thin solid #e7eef1;')
    })
})
