# The MIT License (MIT)
#
# Copyright (c) 2020 ETH Zurich
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.


def generate_plus_grid_isls(
    output_filename_isls,
    n_orbits,
    n_sats_per_orbit,
    even_isl_shift,
    odd_isl_shift,
    idx_offset=0,
    phased=True,
):
    """
    Generate plus grid ISL file.

    :param output_filename_isls     Output filename
    :param n_orbits:                Number of orbits
    :param n_sats_per_orbit:        Number of satellites per orbit
    :param isl_shift:               ISL shift between orbits (e.g., if satellite id in orbit is X,
                                    does it also connect to the satellite at X in the adjacent orbit)
    :param idx_offset:              Index offset (e.g., if you have multiple shells)
    """

    if n_orbits < 3 or n_sats_per_orbit < 3:
        raise ValueError("Number of x and y must each be at least 3")

    list_isls = []
    for i in range(n_orbits):
        for j in range(n_sats_per_orbit):
            sat = i * n_sats_per_orbit + j

            if phased:
                sat_same_orbit = i * n_sats_per_orbit + ((j + 1) % n_sats_per_orbit)

                # Same orbit
                list_isls.append(
                    (
                        idx_offset + min(sat, sat_same_orbit),
                        idx_offset + max(sat, sat_same_orbit),
                    )
                )

                # the adjacent orbit is the one on the right. depending on whether this orbit is even or odd, there
                # may be some assymmetric orbital adjustments
                if i % 2 == 0:
                    sat_adjacent_orbit = ((i + 1) % n_orbits) * n_sats_per_orbit + (
                        (j + even_isl_shift) % n_sats_per_orbit
                    )

                    # due to how phasing works, if this is an even orbit:
                    list_isls.append(
                        (
                            idx_offset + min(sat, sat_adjacent_orbit),
                            idx_offset + max(sat, sat_adjacent_orbit),
                        )
                    )
                else:
                    sat_adjacent_orbit = ((i + 1) % n_orbits) * n_sats_per_orbit + (
                        (j + odd_isl_shift) % n_sats_per_orbit
                    )
                    list_isls.append(
                        (
                            idx_offset + min(sat, sat_adjacent_orbit),
                            idx_offset + max(sat, sat_adjacent_orbit),
                        )
                    )

            else:
                # Link to the next in the orbit
                sat_same_orbit = i * n_sats_per_orbit + ((j + 1) % n_sats_per_orbit)
                sat_adjacent_orbit = ((i + 1) % n_orbits) * n_sats_per_orbit + (
                    (j + even_isl_shift) % n_sats_per_orbit
                )

                # Same orbit
                list_isls.append(
                    (
                        idx_offset + min(sat, sat_same_orbit),
                        idx_offset + max(sat, sat_same_orbit),
                    )
                )

                # Adjacent orbit
                list_isls.append(
                    (
                        idx_offset + min(sat, sat_adjacent_orbit),
                        idx_offset + max(sat, sat_adjacent_orbit),
                    )
                )

    with open(output_filename_isls, "w+") as f:
        for a, b in list_isls:
            f.write(str(a) + " " + str(b) + "\n")

    return list_isls
